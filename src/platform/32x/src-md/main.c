/*
 * SEGA CD Mode 1 Support
 * by Chilly Willy
 */

#include <stdint.h>
#include <string.h>

extern uint32_t vblank_vector;
extern uint16_t gen_lvl2;
extern uint16_t cd_ok;

extern uint32_t Sub_Start;
extern uint32_t Sub_End;

extern void Kos_Decomp(uint8_t *src, uint8_t *dst);

extern void write_byte(unsigned int dst, unsigned char val);
extern void write_word(unsigned int dst, unsigned short val);
extern void write_long(unsigned int dst, unsigned int val);
extern unsigned char read_byte(unsigned int src);
extern unsigned short read_word(unsigned int src);
extern unsigned int read_long(unsigned int src);

extern void do_main(void);

uint16_t InitCD(void)
{
    char *bios;

    /*
     * Check for CD BIOS
     * When a cart is inserted in the MD, the CD hardware is mapped to
     * 0x400000 instead of 0x000000. So the BIOS ROM is at 0x400000, the
     * Program RAM bank is at 0x420000, and the Word RAM is at 0x600000.
     */
    bios = (char *)0x415800;
    if (memcmp(bios + 0x6D, "SEGA", 4))
    {
        bios = (char *)0x416000;
        if (memcmp(bios + 0x6D, "SEGA", 4))
        {
            // check for WonderMega/X'Eye
            if (memcmp(bios + 0x6D, "WONDER", 6))
            {
                bios = (char *)0x41AD00; // might also be 0x40D500
                // check for LaserActive
                if (memcmp(bios + 0x6D, "SEGA", 4))
                    return 0; // no CD
            }
        }
    }

    /*
     * Reset the Gate Array - this specific sequence of writes is recognized by
     * the gate array as a reset sequence, clearing the entire internal state -
     * this is needed for the LaserActive
     */
    write_word(0xA12002, 0xFF00);
    write_byte(0xA12001, 0x03);
    write_byte(0xA12001, 0x02);
    write_byte(0xA12001, 0x00);

    /*
     * Reset the Sub-CPU, request the bus
     */
    write_byte(0xA12001, 0x02);
    while (!(read_byte(0xA12001) & 2)) write_byte(0xA12001, 0x02); // wait on bus acknowledge

    /*
     * Decompress Sub-CPU BIOS to Program RAM at 0x00000
     */
    write_word(0xA12002, 0x0002); // no write-protection, bank 0, 2M mode, Word RAM assigned to Sub-CPU
    memset((char *)0x420000, 0, 0x20000); // clear program ram first bank - needed for the LaserActive
    Kos_Decomp((uint8_t *)bios, (uint8_t *)0x420000);

    /*
     * Copy Sub-CPU program to Program RAM at 0x06000
     */
    memcpy((char *)0x426000, (char *)&Sub_Start, (int)&Sub_End - (int)&Sub_Start);

    write_byte(0xA1200E, 0x00); // clear main comm port
    write_byte(0xA12002, 0x2A); // write-protect up to 0x05400
    write_byte(0xA12001, 0x01); // clear bus request, deassert reset - allow CD Sub-CPU to run
    while (!(read_byte(0xA12001) & 1)) write_byte(0xA12001, 0x01); // wait on Sub-CPU running

    /*
     * Set the vertical blank handler to generate Sub-CPU level 2 ints.
     * The Sub-CPU BIOS needs these in order to run.
     */
    gen_lvl2 = 1; // generate Level 2 IRQ to Sub-CPU

    /*
     * Wait for Sub-CPU program to set sub comm port indicating it is running -
     * note that unless there's something wrong with the hardware, a timeout isn't
     * needed... just loop until the Sub-CPU program responds, but 2000000 is about
     * ten times what the LaserActive needs, and the LA is the slowest unit to
     * initialize
     */
    while (read_byte(0xA1200F) != 'I')
    {
        static int timeout = 0;
        timeout++;
        if (timeout > 2000000)
        {
            gen_lvl2 = 0;
            return 0; // no CD
        }
    }

    /*
     * Wait for Sub-CPU to indicate it is ready to receive commands
     */
    while (read_byte(0xA1200F) != 0x00) ;

    return 1; // CD ready to go!
}

int main(void)
{
    cd_ok = 0; //InitCD();

    /*
     * Main loop in ram - you need to have it in ram to avoid bus contention
     * for the rom with the SH2s.
     */
    do_main(); // never returns

    return 0;
}
