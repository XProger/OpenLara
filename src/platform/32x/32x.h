#ifndef __32X_H__
#define __32X_H__

#define MARS_CRAM           (*(volatile unsigned short *)0x20004200)
#define MARS_FRAMEBUFFER    (*(volatile unsigned short *)0x24000000)
#define MARS_OVERWRITE_IMG  (*(volatile unsigned short *)0x24020000)
#define MARS_SDRAM          (*(volatile unsigned short *)0x26000000)

#define MARS_SYS_INTMSK     (*(volatile unsigned short *)0x20004000)
#define MARS_SYS_DMACTR     (*(volatile unsigned short *)0x20004006)
#define MARS_SYS_DMASAR     (*(volatile unsigned long *)0x20004008)
#define MARS_SYS_DMADAR     (*(volatile unsigned long *)0x2000400C)
#define MARS_SYS_DMALEN     (*(volatile unsigned short *)0x20004010)
#define MARS_SYS_DMAFIFO    (*(volatile unsigned short *)0x20004012)
#define MARS_SYS_VRESI_CLR  (*(volatile unsigned short *)0x20004014)
#define MARS_SYS_VINT_CLR   (*(volatile unsigned short *)0x20004016)
#define MARS_SYS_HINT_CLR   (*(volatile unsigned short *)0x20004018)
#define MARS_SYS_CMDI_CLR   (*(volatile unsigned short *)0x2000401A)
#define MARS_SYS_PWMI_CLR   (*(volatile unsigned short *)0x2000401C)
#define MARS_SYS_COMM0      (*(volatile unsigned short *)0x20004020) /* Master SH2 communication */
#define MARS_SYS_COMM2      (*(volatile unsigned short *)0x20004022)
#define MARS_SYS_COMM4      (*(volatile unsigned short *)0x20004024) /* Slave SH2 communication */
#define MARS_SYS_COMM6      (*(volatile unsigned short *)0x20004026)
#define MARS_SYS_COMM8      (*(volatile unsigned short *)0x20004028) /* controller 1 current value */
#define MARS_SYS_COMM10     (*(volatile unsigned short *)0x2000402A) /* controller 2 current value */
#define MARS_SYS_COMM12     (*(volatile unsigned long *)0x2000402C)  /* vcount current value */

#define MARS_PWM_CTRL       (*(volatile unsigned short *)0x20004030)
#define MARS_PWM_CYCLE      (*(volatile unsigned short *)0x20004032)
#define MARS_PWM_LEFT       (*(volatile unsigned short *)0x20004034)
#define MARS_PWM_RIGHT      (*(volatile unsigned short *)0x20004036)
#define MARS_PWM_MONO       (*(volatile unsigned short *)0x20004038)

#define MARS_VDP_DISPMODE   (*(volatile unsigned short *)0x20004100)
#define MARS_VDP_FILLEN     (*(volatile unsigned short *)0x20004104)
#define MARS_VDP_FILADR     (*(volatile unsigned short *)0x20004106)
#define MARS_VDP_FILDAT     (*(volatile unsigned short *)0x20004108)
#define MARS_VDP_FBCTL      (*(volatile unsigned short *)0x2000410A)

#define MARS_SH2_ACCESS_VDP 0x8000
#define MARS_68K_ACCESS_VDP 0x0000

#define MARS_PAL_FORMAT     0x0000
#define MARS_NTSC_FORMAT    0x8000

#define MARS_VDP_PRIO_68K   0x0000
#define MARS_VDP_PRIO_32X   0x0080

#define MARS_224_LINES      0x0000
#define MARS_240_LINES      0x0040

#define MARS_VDP_MODE_OFF   0x0000
#define MARS_VDP_MODE_256   0x0001
#define MARS_VDP_MODE_32K   0x0002
#define MARS_VDP_MODE_RLE   0x0003

#define MARS_VDP_VBLK       0x8000
#define MARS_VDP_HBLK       0x4000
#define MARS_VDP_PEN        0x2000
#define MARS_VDP_FEN        0x0002
#define MARS_VDP_FS         0x0001

#define SH2_CCTL_W1         0x80
#define SH2_CCTL_W0         0x40
#define SH2_CCTL_CP         0x10
#define SH2_CCTL_TW         0x08
#define SH2_CCTL_OD         0x04
#define SH2_CCTL_ID         0x02
#define SH2_CCTL_CE         0x01

#define SH2_FRT_TIER        (*(volatile unsigned char *)0xFFFFFE10)
#define SH2_FRT_FTCSR       (*(volatile unsigned char *)0xFFFFFE11)
#define SH2_FRT_FRCH        (*(volatile unsigned char *)0xFFFFFE12)
#define SH2_FRT_FRCL        (*(volatile unsigned char *)0xFFFFFE13)
#define SH2_FRT_OCRH        (*(volatile unsigned char *)0xFFFFFE14)
#define SH2_FRT_OCRL        (*(volatile unsigned char *)0xFFFFFE15)
#define SH2_FRT_TCR         (*(volatile unsigned char *)0xFFFFFE16)
#define SH2_FRT_TOCR        (*(volatile unsigned char *)0xFFFFFE17)
#define SH2_FRT_ICRH        (*(volatile unsigned char *)0xFFFFFE18)
#define SH2_FRT_ICRL        (*(volatile unsigned char *)0xFFFFFE19)

#define SH2_DMA_SAR0        (*(volatile unsigned long *)0xFFFFFF80)
#define SH2_DMA_DAR0        (*(volatile unsigned long *)0xFFFFFF84)
#define SH2_DMA_TCR0        (*(volatile unsigned long *)0xFFFFFF88)
#define SH2_DMA_CHCR0       (*(volatile unsigned long *)0xFFFFFF8C)
#define SH2_DMA_VCR0        (*(volatile unsigned long *)0xFFFFFFA0)
#define SH2_DMA_DRCR0       (*(volatile unsigned char *)0xFFFFFE71)

#define SH2_DMA_SAR1        (*(volatile unsigned long *)0xFFFFFF90)
#define SH2_DMA_DAR1        (*(volatile unsigned long *)0xFFFFFF94)
#define SH2_DMA_TCR1        (*(volatile unsigned long *)0xFFFFFF98)
#define SH2_DMA_CHCR1       (*(volatile unsigned long *)0xFFFFFF9C)
#define SH2_DMA_VCR1        (*(volatile unsigned long *)0xFFFFFFA8)
#define SH2_DMA_DRCR1       (*(volatile unsigned char *)0xFFFFFE72)

#define SH2_DMA_DMAOR       (*(volatile unsigned long *)0xFFFFFFB0)

#define SH2_INT_ICR         (*(volatile unsigned short *)0xFFFFFEE0)
#define SH2_INT_IPRA        (*(volatile unsigned short *)0xFFFFFEE2)
#define SH2_INT_IPRB        (*(volatile unsigned short *)0xFFFFFE60)
#define SH2_INT_VCRA        (*(volatile unsigned short *)0xFFFFFE62)
#define SH2_INT_VCRB        (*(volatile unsigned short *)0xFFFFFE64)
#define SH2_INT_VCRC        (*(volatile unsigned short *)0xFFFFFE66)
#define SH2_INT_VCRD        (*(volatile unsigned short *)0xFFFFFE68)
#define SH2_INT_VCRWDT      (*(volatile unsigned short *)0xFFFFFEE4)
#define SH2_INT_VCRDIV      (*(volatile unsigned long *)0xFFFFFF0C)

#define SEGA_CTRL_UP        0x0001
#define SEGA_CTRL_DOWN      0x0002
#define SEGA_CTRL_LEFT      0x0004
#define SEGA_CTRL_RIGHT     0x0008
#define SEGA_CTRL_B         0x0010
#define SEGA_CTRL_C         0x0020
#define SEGA_CTRL_A         0x0040
#define SEGA_CTRL_START     0x0080
#define SEGA_CTRL_Z         0x0100
#define SEGA_CTRL_Y         0x0200
#define SEGA_CTRL_X         0x0400
#define SEGA_CTRL_MODE      0x0800

#define SEGA_CTRL_TYPE      0xF000
#define SEGA_CTRL_THREE     0x0000
#define SEGA_CTRL_SIX       0x1000
#define SEGA_CTRL_NONE      0xF000

#define MASTER_STATUS_OK    1
#define SLAVE_STATUS_OK     2
#define MASTER_LOCK         4
#define SLAVE_LOCK          8

#define SH2_WDT_WTCNT           0x5A00
#define SH2_WDT_WTCSR           (0xA500 | (1 << 3) | (1 << 4))
#define SH2_WDT_WTCSR_CKS_2     0
#define SH2_WDT_WTCSR_CKS_64    1
#define SH2_WDT_WTCSR_CKS_128   2
#define SH2_WDT_WTCSR_CKS_256   3
#define SH2_WDT_WTCSR_CKS_512   4
#define SH2_WDT_WTCSR_CKS_1024  5
#define SH2_WDT_WTCSR_CKS_4096  6
#define SH2_WDT_WTCSR_CKS_8192  7
#define SH2_WDT_WTCSR_TME       (1 << 5)
#define SH2_WDT_WTCSR_WTIT      (1 << 6)
#define SH2_WDT_WTCSR_OVF       (1 << 7)

/* global functions in sh2_crt0.s */
extern "C"
{
    void fast_memcpy(void *dst, const void *src, int len);
    void fast_memset(void *dst, int value, int len);
    void CacheControl(int mode);
    void CacheClearLine(void* ptr);
}

#define CacheClear()\
    CacheControl(0);\
    CacheControl(SH2_CCTL_CP | SH2_CCTL_CE);

#define MARS_WAIT() {while (MARS_SYS_COMM4);}

#endif
