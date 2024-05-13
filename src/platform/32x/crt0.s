!-----------------------------------------------------------------------
! SEGA 32X support code for SH2
! by Chilly Willy
! Rom header and SH2 init/exception code - must be first in object list
!-----------------------------------------------------------------------

        .text

! 68000 exception vector table at 0x000

!-----------------------------------------------------------------------
! Initial exception vectors - when the console is first turned on, it is
! in MegaDrive mode. All vectors just point to the code to start up the
! Mars adapter. After the adapter is enabled, none of these vectors will
! appear as the adapter uses its own vector table to route exceptions to
! the jump table at 0x200.
!-----------------------------------------------------------------------

        .long   0x01000000,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0
        .long   0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0
        .long   0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0
        .long   0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0
        .long   0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0
        .long   0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0
        .long   0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0
        .long   0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0,0x000003F0

! Standard MegaDrive ROM header at 0x100

        .ascii  "SEGA 32X        "      /* First 4 bytes must be "SEGA" */
        .ascii  "XProger (c)2022 "      /* Copyright and date */
        .ascii  "OpenLara Alpha                                  "      /* JP Name */
        .ascii  "OpenLara Alpha                                  "      /* EN Name */
        .ascii  "GM 14021968-01"        /* Serial No. */
        .word   0x0000
        .ascii  "J6              "
        .long   0x00000000,0x005FFFFF   /* ROM start, end */
        .long   0x00FF0000,0x00FFFFFF   /* RAM start, end */

! 2KB of save ram on odd byte lane
        .ascii  "RA"                    /* External RAM */
        .byte   0xF8                    /* don't clear + odd bytes */
        .byte   0x20                    /* SRAM */
        .long   0x00200001,0x00200FFF   /* SRAM start, end */

!       .ascii  "            "          /* no SRAM */

        .ascii  "    "
        .ascii  "                "
        .ascii  "                "
        .ascii  "                "
        .ascii  "F               "      /* enable any hardware configuration */

! Mars 68000 exception jump table at 0x200

        .macro  jump address
        .word   0x4EF9,\address>>16,\address&0xFFFF
        .endm

        .macro  call address
        .word   0x4EB9,\address>>16,\address&0xFFFF
        .endm

        jump    0x880800    /* reset = hot start */
        call    0x880840    /* EX_BusError */
        call    0x880840    /* EX_AddrError */
        call    0x880840    /* EX_IllInstr */
        call    0x880840    /* EX_DivByZero */
        call    0x880840    /* EX_CHK */
        call    0x880840    /* EX_TrapV */
        call    0x880840    /* EX_Priviledge */
        call    0x880840    /* EX_Trace */
        call    0x880840    /* EX_LineA */
        call    0x880840    /* EX_LineF */
        .space  72          /* reserved */
        call    0x880840    /* EX_Spurious */
        call    0x880840    /* EX_Level1 */
        call    0x880840    /* EX_Level2 */
        call    0x880840    /* EX_Level3 */
        jump    0x880880    /* EX_Level4 HBlank */
        call    0x880840    /* EX_Level5 */
        jump    0x8808C0    /* EX_Level6 VBlank */
        call    0x880840    /* EX_Level7 */
        call    0x880840    /* EX_Trap0 */
        call    0x880840    /* EX_Trap1 */
        call    0x880840    /* EX_Trap2 */
        call    0x880840    /* EX_Trap3 */
        call    0x880840    /* EX_Trap4 */
        call    0x880840    /* EX_Trap5 */
        call    0x880840    /* EX_Trap6 */
        call    0x880840    /* EX_Trap7 */
        call    0x880840    /* EX_Trap8 */
        call    0x880840    /* EX_Trap9 */
        call    0x880840    /* EX_TrapA */
        call    0x880840    /* EX_TrapB */
        call    0x880840    /* EX_TrapC */
        call    0x880840    /* EX_TrapD */
        call    0x880840    /* EX_TrapE */
        call    0x880840    /* EX_TrapF */
        .space  166         /* reserved */

! Standard Mars Header at 0x3C0

        .ascii  "OpenLara        "      /* module name */
        .long   0x00000000              /* version */
        .long   __text_end-0x02000000   /* Source (in ROM) */
        .long   0x00000000              /* Destination (in SDRAM) */
        .long   __data_size             /* Size */
        .long   pri_start               /* Primary SH2 Jump */
        .long   sec_start               /* Secondary SH2 Jump */
        .long   pri_vbr                 /* Primary SH2 VBR */
        .long   sec_vbr                 /* Secondary SH2 VBR */

! Standard 32X startup code for MD side at 0x3F0

        .word   0x287C,0xFFFF,0xFFC0,0x23FC,0x0000,0x0000,0x00A1,0x5128
        .word   0x46FC,0x2700,0x4BF9,0x00A1,0x0000,0x7001,0x0CAD,0x4D41
        .word   0x5253,0x30EC,0x6600,0x03E6,0x082D,0x0007,0x5101,0x67F8
        .word   0x4AAD,0x0008,0x6710,0x4A6D,0x000C,0x670A,0x082D,0x0000
        .word   0x5101,0x6600,0x03B8,0x102D,0x0001,0x0200,0x000F,0x6706
        .word   0x2B78,0x055A,0x4000,0x7200,0x2C41,0x4E66,0x41F9,0x0000
        .word   0x04D4,0x6100,0x0152,0x6100,0x0176,0x47F9,0x0000,0x04E8
        .word   0x43F9,0x00A0,0x0000,0x45F9,0x00C0,0x0011,0x3E3C,0x0100
        .word   0x7000,0x3B47,0x1100,0x3B47,0x1200,0x012D,0x1100,0x66FA
        .word   0x7425,0x12DB,0x51CA,0xFFFC,0x3B40,0x1200,0x3B40,0x1100
        .word   0x3B47,0x1200,0x149B,0x149B,0x149B,0x149B,0x41F9,0x0000
        .word   0x04C0,0x43F9,0x00FF,0x0000,0x22D8,0x22D8,0x22D8,0x22D8
        .word   0x22D8,0x22D8,0x22D8,0x22D8,0x41F9,0x00FF,0x0000,0x4ED0
        .word   0x1B7C,0x0001,0x5101,0x41F9,0x0000,0x06BC,0xD1FC,0x0088
        .word   0x0000,0x4ED0,0x0404,0x303C,0x076C,0x0000,0x0000,0xFF00
        .word   0x8137,0x0002,0x0100,0x0000,0xAF01,0xD91F,0x1127,0x0021
        .word   0x2600,0xF977,0xEDB0,0xDDE1,0xFDE1,0xED47,0xED4F,0xD1E1
        .word   0xF108,0xD9C1,0xD1E1,0xF1F9,0xF3ED,0x5636,0xE9E9,0x9FBF
        .word   0xDFFF,0x4D41,0x5253,0x2049,0x6E69,0x7469,0x616C,0x2026
        .word   0x2053,0x6563,0x7572,0x6974,0x7920,0x5072,0x6F67,0x7261
        .word   0x6D20,0x2020,0x2020,0x2020,0x2020,0x2043,0x6172,0x7472
        .word   0x6964,0x6765,0x2056,0x6572,0x7369,0x6F6E,0x2020,0x2020
        .word   0x436F,0x7079,0x7269,0x6768,0x7420,0x5345,0x4741,0x2045
        .word   0x4E54,0x4552,0x5052,0x4953,0x4553,0x2C4C,0x5444,0x2E20
        .word   0x3139,0x3934,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020
        .word   0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020
        .word   0x2020,0x2020,0x2020,0x524F,0x4D20,0x5665,0x7273,0x696F
        .word   0x6E20,0x312E,0x3000,0x48E7,0xC040,0x43F9,0x00C0,0x0004
        .word   0x3011,0x303C,0x8000,0x323C,0x0100,0x3E3C,0x0012,0x1018
        .word   0x3280,0xD041,0x51CF,0xFFF8,0x4CDF,0x0203,0x4E75,0x48E7
        .word   0x81C0,0x41F9,0x0000,0x063E,0x43F9,0x00C0,0x0004,0x3298
        .word   0x3298,0x3298,0x3298,0x3298,0x3298,0x3298,0x2298,0x3341
        .word   0xFFFC,0x3011,0x0800,0x0001,0x66F8,0x3298,0x3298,0x7000
        .word   0x22BC,0xC000,0x0000,0x7E0F,0x3340,0xFFFC,0x3340,0xFFFC
        .word   0x3340,0xFFFC,0x3340,0xFFFC,0x51CF,0xFFEE,0x22BC,0x4000
        .word   0x0010,0x7E09,0x3340,0xFFFC,0x3340,0xFFFC,0x3340,0xFFFC
        .word   0x3340,0xFFFC,0x51CF,0xFFEE,0x4CDF,0x0381,0x4E75,0x8114
        .word   0x8F01,0x93FF,0x94FF,0x9500,0x9600,0x9780,0x4000,0x0080
        .word   0x8104,0x8F02,0x48E7,0xC140,0x43F9,0x00A1,0x5180,0x08A9
        .word   0x0007,0xFF80,0x66F8,0x3E3C,0x00FF,0x7000,0x7200,0x337C
        .word   0x00FF,0x0004,0x3341,0x0006,0x3340,0x0008,0x4E71,0x0829
        .word   0x0001,0x000B,0x66F8,0x0641,0x0100,0x51CF,0xFFE8,0x4CDF
        .word   0x0283,0x4E75,0x48E7,0x8180,0x41F9,0x00A1,0x5200,0x08A8
        .word   0x0007,0xFF00,0x66F8,0x3E3C,0x001F,0x20C0,0x20C0,0x20C0
        .word   0x20C0,0x51CF,0xFFF6,0x4CDF,0x0181,0x4E75,0x41F9,0x00FF
        .word   0x0000,0x3E3C,0x07FF,0x7000,0x20C0,0x20C0,0x20C0,0x20C0
        .word   0x20C0,0x20C0,0x20C0,0x20C0,0x51CF,0xFFEE,0x3B7C,0x0000
        .word   0x1200,0x7E0A,0x51CF,0xFFFE,0x43F9,0x00A1,0x5100,0x7000
        .word   0x2340,0x0020,0x2340,0x0024,0x1B7C,0x0003,0x5101,0x2E79
        .word   0x0088,0x0000,0x0891,0x0007,0x66FA,0x7000,0x3340,0x0002
        .word   0x3340,0x0004,0x3340,0x0006,0x2340,0x0008,0x2340,0x000C
        .word   0x3340,0x0010,0x3340,0x0030,0x3340,0x0032,0x3340,0x0038
        .word   0x3340,0x0080,0x3340,0x0082,0x08A9,0x0000,0x008B,0x66F8
        .word   0x6100,0xFF12,0x08E9,0x0000,0x008B,0x67F8,0x6100,0xFF06
        .word   0x08A9,0x0000,0x008B,0x6100,0xFF3C,0x303C,0x0040,0x2229
        .word   0x0020,0x0C81,0x5351,0x4552,0x6700,0x0092,0x303C,0x0080
        .word   0x2229,0x0020,0x0C81,0x5344,0x4552,0x6700,0x0080,0x21FC
        .word   0x0088,0x02A2,0x0070,0x303C,0x0002,0x7200,0x122D,0x0001
        .word   0x1429,0x0080,0xE14A,0x8242,0x0801,0x000F,0x660A,0x0801
        .word   0x0006,0x6700,0x0058,0x6008,0x0801,0x0006,0x6600,0x004E
        .word   0x7020,0x41F9,0x0088,0x0000,0x3C28,0x018E,0x4A46,0x6700
        .word   0x0010,0x3429,0x0028,0x0C42,0x0000,0x67F6,0xB446,0x662C
        .word   0x7000,0x2340,0x0028,0x2340,0x002C,0x3E14,0x2C7C,0xFFFF
        .word   0xFFC0,0x4CD6,0x7FF9,0x44FC,0x0000,0x6014,0x43F9,0x00A1
        .word   0x5100,0x3340,0x0006,0x303C,0x8000,0x6004,0x44FC,0x0001

!-----------------------------------------------------------------------
! At this point (0x800), the Work RAM is clear, the VDP initialized, the
! VRAM/VSRAM/CRAM cleared, the Z80 initialized, the 32X initialized,
! both 32X framebuffers cleared, the 32X palette cleared, the SH2s
! checked for a startup error, the adapter TV mode matches the MD TV
! mode, and the ROM checksum checked. If any error is detected, the
! carry is set, otherwise it is cleared. The 68000 main code is now
! entered.
!-----------------------------------------------------------------------

        .incbin "src-md/m68k.bin"       /* all 68000 code & data, compiled to 0x880800/0xFF0000 */


        .global _gLightmap_base
        .global _gLightmap

        .data
_gLightmap_base:
        .space  128
_gLightmap:
        .space  256 * 32

!-----------------------------------------------------------------------
! Primary Vector Base Table
!-----------------------------------------------------------------------

        .equ    pri_stack, 0x0603F800
        .equ    sec_stack, 0x06040000

        .align  4
pri_vbr:
        .long   pri_start       /* Cold Start PC */
        .long   pri_stack       /* Cold Start SP */
        .long   pri_start       /* Manual Reset PC */
        .long   pri_stack       /* Manual Reset SP */
        .long   pri_err         /* Illegal instruction */
        .long   0x00000000      /* reserved */
        .long   pri_err         /* Invalid slot instruction */
        .long   0x00000000      /* reserved */
        .long   0x00000000      /* reserved */
        .long   pri_err         /* CPU address error */
        .long   pri_err         /* DMA address error */
        .long   pri_err         /* NMI vector */
        .long   pri_err         /* User break vector */
        .space  76              /* reserved */
        .long   pri_err         /* TRAPA #32 */
        .long   pri_err         /* TRAPA #33 */
        .long   pri_err         /* TRAPA #34 */
        .long   pri_err         /* TRAPA #35 */
        .long   pri_err         /* TRAPA #36 */
        .long   pri_err         /* TRAPA #37 */
        .long   pri_err         /* TRAPA #38 */
        .long   pri_err         /* TRAPA #39 */
        .long   pri_err         /* TRAPA #40 */
        .long   pri_err         /* TRAPA #41 */
        .long   pri_err         /* TRAPA #42 */
        .long   pri_err         /* TRAPA #43 */
        .long   pri_err         /* TRAPA #44 */
        .long   pri_err         /* TRAPA #45 */
        .long   pri_err         /* TRAPA #46 */
        .long   pri_err         /* TRAPA #47 */
        .long   pri_err         /* TRAPA #48 */
        .long   pri_err         /* TRAPA #49 */
        .long   pri_err         /* TRAPA #50 */
        .long   pri_err         /* TRAPA #51 */
        .long   pri_err         /* TRAPA #52 */
        .long   pri_err         /* TRAPA #53 */
        .long   pri_err         /* TRAPA #54 */
        .long   pri_err         /* TRAPA #55 */
        .long   pri_err         /* TRAPA #56 */
        .long   pri_err         /* TRAPA #57 */
        .long   pri_err         /* TRAPA #58 */
        .long   pri_err         /* TRAPA #59 */
        .long   pri_err         /* TRAPA #60 */
        .long   pri_err         /* TRAPA #61 */
        .long   pri_err         /* TRAPA #62 */
        .long   pri_err         /* TRAPA #63 */
        .long   pri_irq         /* FRT interrupt (Level 1) */
        .long   pri_irq         /* WDT interrupt (Level 2 & 3) */
        .long   pri_irq         /* DMA interrupt (Level 4 & 5) */
        .long   pri_irq         /* PWM interupt (Level 6 & 7) */
        .long   pri_irq         /* Command interupt (Level 8 & 9) */
        .long   pri_irq         /* H Blank interupt (Level 10 & 11) */
        .long   pri_irq         /* V Blank interupt (Level 12 & 13) */
        .long   pri_irq         /* Reset Button (Level 14 & 15) */

!-----------------------------------------------------------------------
! Secondary Vector Base Table
!-----------------------------------------------------------------------

sec_vbr:
        .long   sec_start       /* Cold Start PC */
        .long   sec_stack       /* Cold Start SP */
        .long   sec_start       /* Manual Reset PC */
        .long   sec_stack       /* Manual Reset SP */
        .long   sec_err         /* Illegal instruction */
        .long   0x00000000      /* reserved */
        .long   sec_err         /* Invalid slot instruction */
        .long   0x00000000      /* reserved */
        .long   0x00000000      /* reserved */
        .long   sec_err         /* CPU address error */
        .long   sec_err         /* DMA address error */
        .long   sec_err         /* NMI vector */
        .long   sec_err         /* User break vector */
        .space  76              /* reserved */
        .long   sec_err         /* TRAPA #32 */
        .long   sec_err         /* TRAPA #33 */
        .long   sec_err         /* TRAPA #34 */
        .long   sec_err         /* TRAPA #35 */
        .long   sec_err         /* TRAPA #36 */
        .long   sec_err         /* TRAPA #37 */
        .long   sec_err         /* TRAPA #38 */
        .long   sec_err         /* TRAPA #39 */
        .long   sec_err         /* TRAPA #40 */
        .long   sec_err         /* TRAPA #41 */
        .long   sec_err         /* TRAPA #42 */
        .long   sec_err         /* TRAPA #43 */
        .long   sec_err         /* TRAPA #44 */
        .long   sec_err         /* TRAPA #45 */
        .long   sec_err         /* TRAPA #46 */
        .long   sec_err         /* TRAPA #47 */
        .long   sec_err         /* TRAPA #48 */
        .long   sec_err         /* TRAPA #49 */
        .long   sec_err         /* TRAPA #50 */
        .long   sec_err         /* TRAPA #51 */
        .long   sec_err         /* TRAPA #52 */
        .long   sec_err         /* TRAPA #53 */
        .long   sec_err         /* TRAPA #54 */
        .long   sec_err         /* TRAPA #55 */
        .long   sec_err         /* TRAPA #56 */
        .long   sec_err         /* TRAPA #57 */
        .long   sec_err         /* TRAPA #58 */
        .long   sec_err         /* TRAPA #59 */
        .long   sec_err         /* TRAPA #60 */
        .long   sec_err         /* TRAPA #61 */
        .long   sec_err         /* TRAPA #62 */
        .long   sec_err         /* TRAPA #63 */
        .long   sec_irq         /* FRT interrupt (Level 1) */
        .long   sec_irq         /* WDT interrupt (Level 2 & 3) */
        .long   sec_irq         /* DMA interrupt (Level 4 & 5) */
        .long   sec_irq         /* PWM interupt (Level 6 & 7) */
        .long   sec_irq         /* Command interupt (Level 8 & 9) */
        .long   sec_irq         /* H Blank interupt (Level 10 & 11 */
        .long   sec_irq         /* V Blank interupt (Level 12 & 13) */
        .long   sec_irq         /* Reset Button (Level 14 & 15) */

!-----------------------------------------------------------------------
! The Primary SH2 starts here
!-----------------------------------------------------------------------

pri_start:
        ! clear interrupt flags
        mov.l   _pri_int_clr,r1
        mov.w   r0,@-r1                 /* PWM INT clear */
        mov.w   r0,@r1
        mov.w   r0,@-r1                 /* CMD INT clear */
        mov.w   r0,@r1
        mov.w   r0,@-r1                 /* H INT clear */
        mov.w   r0,@r1
        mov.w   r0,@-r1                 /* V INT clear */
        mov.w   r0,@r1
        mov.w   r0,@-r1                 /* VRES INT clear */
        mov.w   r0,@r1

        mov.l   _pri_sh2_frtctl,r1      /* Set Free Run Timer */
        mov     #0x00,r0
        mov.b   r0,@(0x00,r1)           /* TIER = ints disabled */
        mov     #0xE2,r0
        mov.b   r0,@(0x07,r1)           /* TOCR = select OCRA, output 1 on compare match */
        mov     #0x00,r0
        mov.b   r0,@(0x04,r1)           /* OCR_H */
        mov     #0x01,r0
        mov.b   r0,@(0x05,r1)           /* OCR_L => OCRA = 0x0001 */
        mov     #0,r0
        mov.b   r0,@(0x06,r1)           /* TCR = input captured on falling edge, CKS = Fs/8 */
        mov     #1,r0
        mov.b   r0,@(0x01,r1)           /* TCSR = clear FRC on match OCRA */
        mov     #0x00,r0
        mov.b   r0,@(0x03,r1)           /* FRC_L */
        mov.b   r0,@(0x02,r1)           /* FRC_H => clear FRC */

        mov.l   _pri_stk,r15

        ! purge cache and turn it off
        mov.l   _pri_cctl,r0
        mov     #0x10,r1                /* CP = cache purge, /CE = cache disabled */
        mov.b   r1,@r0

        ! clear bss
        mov     #0,r0
        mov.l   _bss_dst,r1
        mov.l   _bss_end,r2
0:
        mov.b   r0,@r1
        add     #1,r1
        cmp/eq  r1,r2
        bf      0b

        ! wait for 68000 to finish init
        mov.l   _pri_sts,r0
        mov.l   _pri_ok,r1
1:
        mov.l   @r0,r2
        nop
        nop
        cmp/eq  r1,r2
        bt      1b

        ! let Secondary SH2 run
        mov     #0,r1
        mov.l   r1,@(4,r0)              /* clear secondary status */

        mov     #0x80,r0
        mov.l   _pri_adapter,r1
        mov.b   r0,@r1                  /* set FM */
        mov     #0x08,r0                /* vbi enabled */
        mov.b   r0,@(1,r1)              /* set int enables */
        mov     #0x10,r0
        ldc     r0,sr                   /* allow ints */

        ! purge cache, turn it on, and run main()
        mov.l   _pri_cctl,r0
        mov     #0x11,r1                /* CP = cache purge, CE = cache enabled */
        mov.b   r1,@r0

        mov.l   _pri_go,r0
        jmp     @r0
        nop

        .align   2
_pri_int_clr:
        .long   0x2000401E              /* one word passed last int clr reg */
_pri_stk:
        .long   pri_stack               /* Cold Start SP */
_pri_sts:
        .long   0x20004020
_pri_sh2_frtctl:
        .long   0xfffffe10
_pri_ok:
        .ascii  "M_OK"
_pri_adapter:
        .long   0x20004000
_pri_cctl:
        .long   0xFFFFFE92
_pri_go:
        .long   _main

_bss_dst:
        .long   __bss_start
_bss_end:
        .long   __bss_end

!-----------------------------------------------------------------------
! Primary exception handler
!-----------------------------------------------------------------------

pri_err:
        rte
        nop

!-----------------------------------------------------------------------
! Primary IRQ handler
!-----------------------------------------------------------------------

pri_irq:
        mov.l   r0,@-r15
        mov.l   r1,@-r15
        mov.l   r2,@-r15

        stc     sr,r1                   /* SR holds IRQ level in I3-I0 */
        mov.w   p_int_off,r2
        ldc     r2,sr                   /* disallow ints */

        mov.l   p_sys_frt_tocr,r2
        mov     #0xE0,r0                /* TOCR = select OCRA, output 0 on compare match */
        mov.b   r0,@r2
        mov.b   @r2,r0

        sts.l   pr,@-r15
        mov     r1,r0
        shlr2   r0
        and     #0x3C,r0                /* int level to table offset */
        mov.l   p_int_jtable,r1
        mov.l   @(r0,r1),r0
        jsr     @r0
        nop

        lds.l   @r15+,pr
        mov.l   @r15+,r2
        mov.l   @r15+,r1
        mov.l   @r15+,r0
        rte
        nop

        .align  2
p_sys_frt_tocr:
        .long   0xFFFFFE17
p_int_jtable:
        .long   _p_int_jtable
p_int_off:
        .word   0x00F0

        .align  4
_p_int_jtable:
        .long   pri_no_irq              /* level 0 (ILL) */
        .long   pri_no_irq              /* level 1 (FRT) */
        .long   pri_wdt_irq             /* level 2 (WDT) */
        .long   pri_wdt_irq             /* level 3 (WDT) */
        .long   pri_dma_irq             /* level 4 (DMA) */
        .long   pri_dma_irq             /* level 5 (DMA) */
        .long   pri_pwm_irq             /* level 6 (PWM) */
        .long   pri_pwm_irq             /* level 7 (PWM) */
        .long   pri_cmd_irq             /* level 8 (CMD) */
        .long   pri_cmd_irq             /* level 9 (CMD) */
        .long   pri_h_irq               /* level 10 (HBI) */
        .long   pri_h_irq               /* level 11 (HBI) */
        .long   pri_v_irq               /* level 12 (VBI) */
        .long   pri_v_irq               /* level 13 (VBI) */
        .long   pri_vres_irq            /* level 14 (VRES) */
        .long   pri_vres_irq            /* level 15 (VRES) */

!-----------------------------------------------------------------------
! Primary No IRQ handler
!-----------------------------------------------------------------------

pri_no_irq:
        rts
        nop

!-----------------------------------------------------------------------
! Primary V Blank IRQ handler
!-----------------------------------------------------------------------

pri_v_irq:
        ! bump ints if necessary
        mov.l   pvi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   pvi_mars_adapter,r1
        mov.w   r0,@(0x16,r1)           /* clear V IRQ */

        ! handle V IRQ - save registers
        sts.l   pr,@-r15
        mov.l   r3,@-r15
        mov.l   r4,@-r15
        mov.l   r5,@-r15
        mov.l   r6,@-r15
        mov.l   r7,@-r15
        sts.l   mach,@-r15
        sts.l   macl,@-r15

        mov.l   pvbi_handler_ptr,r0
        jsr     @r0
        nop

        ! restore registers
        lds.l   @r15+,macl
        lds.l   @r15+,mach
        mov.l   @r15+,r7
        mov.l   @r15+,r6
        mov.l   @r15+,r5
        mov.l   @r15+,r4
        mov.l   @r15+,r3
        lds.l   @r15+,pr
        rts
        nop

        .align  2
pvi_mars_adapter:
        .long   0x20004000
pvbi_handler_ptr:
        .long   _pri_vbi_handler
pvi_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Primary H Blank IRQ handler
!-----------------------------------------------------------------------

pri_h_irq:
        ! bump ints if necessary
        mov.l   phi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   phi_mars_adapter,r1
        mov.w   r0,@(0x18,r1)           /* clear H IRQ */
        nop
        nop
        nop
        nop

        ! handle H IRQ (remove nops if more than 8 cycles)

        rts
        nop

        .align  2
phi_mars_adapter:
        .long   0x20004000
phi_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Primary Command IRQ handler
!-----------------------------------------------------------------------

pri_cmd_irq:
        ! bump ints if necessary
        mov.l   pci_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   pci_mars_adapter,r1
        mov.w   r0,@(0x1A,r1)           /* clear CMD IRQ */
        nop
        nop
        nop
        nop

        ! handle CMD IRQ (remove nops if more than 8 cycles)

        rts
        nop

        .align  2
pci_mars_adapter:
        .long   0x20004000
pci_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Primary PWM IRQ handler
!-----------------------------------------------------------------------

pri_pwm_irq:
        ! bump ints if necessary
        mov.l   ppi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   ppi_mars_adapter,r1
        mov.w   r0,@(0x1C,r1)           /* clear PWM IRQ */
        nop
        nop
        nop
        nop

        ! handle PWM IRQ (remove nops if more than 8 cycles)

        rts
        nop

        .align  2
ppi_mars_adapter:
        .long   0x20004000
ppi_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Primary DMA IRQ handler
!-----------------------------------------------------------------------

pri_dma_irq:
        ! bump ints if necessary
        mov.l   pdi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        ! handle DMA IRQ

        rts
        nop

        .align  2
pdi_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Primary WDT IRQ handler
!-----------------------------------------------------------------------

pri_wdt_irq:
        ! bump ints if necessary
        mov.l   pwi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   pwi_sh2_wdtctl,r1
        mov.b   @r1,r0                  /* read WTCSR */
        tst     #0x80,r0                /* check OVF */
        bt      1f                      /* no overflow */
        mov.w   pwi_clr_ovf,r0
        mov.w   r0,@r1                  /* clear OVF */

        ! handle WDT overflow
        mov.l   pwi_ovf_count,r1
        mov.l   @r1,r0
        add     #1,r0
        mov.l   r0,@r1
1:
        rts
        nop

        .align  2
pwi_sh2_frtctl:
        .long   0xfffffe10
pwi_sh2_wdtctl:
        .long   0xfffffe80
pwi_ovf_count:
        .long   _mars_pwdt_ovf_count
pwi_clr_ovf:
        .word   0xa53e                  /* A5 = sel WTCSR, 3E = clr OVF, IT mode, timer enabled, clksel = Fs/4096 */

!-----------------------------------------------------------------------
! Primary RESET IRQ handler
!-----------------------------------------------------------------------

pri_vres_irq:
        mov.l   pvri_mars_adapter,r1
        mov.w   r0,@(0x14,r1)           /* clear VRES IRQ */

        mov     #0x0F,r0
        shll2   r0
        shll2   r0
        ldc     r0,sr                   /* disallow ints */

        mov.l   pvri_pri_stk,r15
        mov.l   pvri_pri_vres,r0
        jmp     @r0
        nop

        .align  2
pvri_mars_adapter:
        .long   0x20004000
pvri_pri_stk:
        .long   pri_stack               /* Cold Start SP */
pvri_pri_vres:
        .long   pri_reset

!-----------------------------------------------------------------------
! The Secondary SH2 starts here
!-----------------------------------------------------------------------

sec_start:
        ! clear interrupt flags
        mov.l   _sec_int_clr,r1
        mov.w   r0,@-r1                 /* PWM INT clear */
        mov.w   r0,@r1
        mov.w   r0,@-r1                 /* CMD INT clear */
        mov.w   r0,@r1
        mov.w   r0,@-r1                 /* H INT clear */
        mov.w   r0,@r1
        mov.w   r0,@-r1                 /* V INT clear */
        mov.w   r0,@r1
        mov.w   r0,@-r1                 /* VRES INT clear */
        mov.w   r0,@r1

        mov.l   _sec_sh2_frtctl,r1      /* Set Free Run Timer */
        mov     #0x00,r0
        mov.b   r0,@(0x00,r1)           /* TIER = ints disabled */
        mov     #0xE2,r0
        mov.b   r0,@(0x07,r1)           /* TOCR = select OCRA, output 1 on compare match */
        mov     #0x00,r0
        mov.b   r0,@(0x04,r1)           /* OCR_H */
        mov     #0x01,r0
        mov.b   r0,@(0x05,r1)           /* OCR_L => OCRA = 0x0001 */
        mov     #0,r0
        mov.b   r0,@(0x06,r1)           /* TCR = input captured on falling edge, CKS = Fs/8 */
        mov     #1,r0
        mov.b   r0,@(0x01,r1)           /* TCSR = clear FRC on match OCRA */
        mov     #0x00,r0
        mov.b   r0,@(0x03,r1)           /* FRC_L */
        mov.b   r0,@(0x02,r1)           /* FRC_H => clear FRC */

        mov.l   _sec_stk,r15

        ! wait for Primary SH2 and 68000 to finish init
        mov.l   _sec_sts,r0
        mov.l   _sec_ok,r1
1:
        mov.l   @r0,r2
        nop
        nop
        cmp/eq  r1,r2
        bt      1b

        mov.l   _sec_adapter,r1
        mov     #0x00,r0
        mov.b   r0,@(1,r1)              /* set int enables (different from primary despite same address!) */
        mov     #0x0D,r0
        shll2   r0
        shll2   r0
        ldc     r0,sr                   /* disallow ints */

! purge cache, turn it on, and run secondary()
        mov.l   _sec_cctl,r0
        mov     #0x11,r1                /* CP = cache purge, CE = cache enabled */
        mov.b   r1,@r0

        mov.l   _sec_go,r0
        jmp     @r0
        nop

        .align   2
_sec_int_clr:
        .long   0x2000401E              /* one word passed last int clr reg */
_sec_stk:
        .long   sec_stack               /* Cold Start SP */
_sec_sts:
        .long   0x20004024
_sec_sh2_frtctl:
        .long   0xfffffe10
_sec_ok:
        .ascii  "S_OK"
_sec_adapter:
        .long   0x20004000
_sec_cctl:
        .long   0xFFFFFE92
_sec_go:
        .long   _secondary

!-----------------------------------------------------------------------
! Secondary exception handler
!-----------------------------------------------------------------------

sec_err:
        rte
        nop

!-----------------------------------------------------------------------
! Secondary IRQ handler
!-----------------------------------------------------------------------

sec_irq:
        mov.l   r0,@-r15
        mov.l   r1,@-r15
        mov.l   r2,@-r15

        stc     sr,r1                   /* SR holds IRQ level in I3-I0 */
        mov.w   s_int_off,r2
        ldc     r2,sr                   /* disallow ints */

        mov.l   s_sys_frt_tocr,r2
        mov     #0xE0,r0                /* TOCR = select OCRA, output 0 on compare match */
        mov.b   r0,@r2
        mov.b   @r2,r0

        sts.l   pr,@-r15
        mov     r1,r0
        shlr2   r0
        and     #0x3C,r0                /* int level to table offset */
        mov.l   s_int_jtable,r1
        mov.l   @(r0,r1),r0
        jsr     @r0
        nop

        lds.l   @r15+,pr
        mov.l   @r15+,r2
        mov.l   @r15+,r1
        mov.l   @r15+,r0
        rte
        nop

        .align  2
s_sys_frt_tocr:
        .long   0xFFFFFE17
s_int_jtable:
        .long   _s_int_jtable
s_int_off:
        .word   0x00F0

        .align  4
_s_int_jtable:
        .long   sec_no_irq              /* level 0 (ILL) */
        .long   sec_no_irq              /* level 1 (FRT) */
        .long   sec_wdt_irq             /* level 2 (WDT) */
        .long   sec_wdt_irq             /* level 3 (WDT) */
        .long   sec_dma_irq             /* level 4 (DMA) */
        .long   sec_dma_irq             /* level 5 (DMA) */
        .long   sec_pwm_irq             /* level 6 (PWM) */
        .long   sec_pwm_irq             /* level 7 (PWM) */
        .long   sec_cmd_irq             /* level 8 (CMD) */
        .long   sec_cmd_irq             /* level 9 (CMD) */
        .long   sec_h_irq               /* level 10 (HBI) */
        .long   sec_h_irq               /* level 11 (HBI) */
        .long   sec_v_irq               /* level 12 (VBI) */
        .long   sec_v_irq               /* level 13 (VBI) */
        .long   sec_vres_irq            /* level 14 (VRES) */
        .long   sec_vres_irq            /* level 15 (VRES) */

!-----------------------------------------------------------------------
! Secondary No IRQ handler
!-----------------------------------------------------------------------

sec_no_irq:
        rts
        nop

!-----------------------------------------------------------------------
! Secondary V Blank IRQ handler
!-----------------------------------------------------------------------

sec_v_irq:
        ! bump ints if necessary
        mov.l   svi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   svi_mars_adapter,r1
        mov.w   r0,@(0x16,r1)           /* clear V IRQ */
        nop
        nop
        nop
        nop

        ! handle V IRQ (remove nops if more than 8 cycles)

        rts
        nop

        .align  2
svi_mars_adapter:
        .long   0x20004000
svi_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Secondary H Blank IRQ handler
!-----------------------------------------------------------------------

sec_h_irq:
        ! bump ints if necessary
        mov.l   shi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   shi_mars_adapter,r1
        mov.w   r0,@(0x18,r1)           /* clear H IRQ */
        nop
        nop
        nop
        nop

        ! handle H IRQ (remove nops if more than 8 cycles)

        rts
        nop

        .align  2
shi_mars_adapter:
        .long   0x20004000
shi_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Secondary Command IRQ handler
!-----------------------------------------------------------------------

sec_cmd_irq:
        ! bump ints if necessary
        mov.l   sci_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   sci_mars_adapter,r1
        mov.w   r0,@(0x1A,r1)           /* clear CMD IRQ */
        nop
        nop
        nop
        nop

        ! handle CMD IRQ (remove nops if more than 8 cycles)

        rts
        nop

        .align  2
sci_mars_adapter:
        .long   0x20004000
sci_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Secondary PWM IRQ handler
!-----------------------------------------------------------------------

sec_pwm_irq:
        ! bump ints if necessary
        mov.l   spi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   spi_mars_adapter,r1
        mov.w   r0,@(0x1C,r1)           /* clear PWM IRQ */
        nop
        nop
        nop
        nop

        ! handle PWM IRQ (remove nops if more than 8 cycles)

        rts
        nop

        .align  2
spi_mars_adapter:
        .long   0x20004000
spi_sh2_frtctl:
        .long   0xfffffe10

!-----------------------------------------------------------------------
! Secondary DMA IRQ handler
!-----------------------------------------------------------------------

sec_dma_irq:
        ! bump ints if necessary
        mov.l   sdi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        ! handle DMA IRQ
        sts.l   pr,@-r15
        mov.l   r3,@-r15
        mov.l   r4,@-r15
        mov.l   r5,@-r15
        mov.l   r6,@-r15
        mov.l   r7,@-r15
        sts.l   mach,@-r15
        sts.l   macl,@-r15

        mov.l   sdi_dma_handler,r0
        jsr     @r0
        nop

        ! restore registers
        lds.l   @r15+,macl
        lds.l   @r15+,mach
        mov.l   @r15+,r7
        mov.l   @r15+,r6
        mov.l   @r15+,r5
        mov.l   @r15+,r4
        mov.l   @r15+,r3
        lds.l   @r15+,pr

        rts
        nop

        .align  2
sdi_sh2_frtctl:
        .long   0xfffffe10
sdi_dma_handler:
        .long   _sec_dma1_handler

!-----------------------------------------------------------------------
! Secondary WDT IRQ handler
!-----------------------------------------------------------------------

sec_wdt_irq:
        ! bump ints if necessary
        mov.l   swi_sh2_frtctl,r1
        mov     #0xE2,r0                /* TOCR = select OCRA, output 1 on compare match */
        mov.b   r0,@(0x07,r1)           /* write TOCR */
        mov.b   @(0x07,r1),r0           /* read TOCR */

        mov.l   swi_sh2_wdtctl,r1
        mov.b   @r1,r0                  /* read WTCSR */
        tst     #0x80,r0                /* check OVF */
        bt      1f                      /* no overflow */
        mov.w   swi_clr_ovf,r0
        mov.w   r0,@r1                  /* clear OVF */

        ! handle WDT overflow
        mov.l   swi_ovf_count,r1
        mov.l   @r1,r0
        add     #1,r0
        mov.l   r0,@r1
1:
        rts
        nop

        .align  2
swi_sh2_frtctl:
        .long   0xfffffe10
swi_sh2_wdtctl:
        .long   0xfffffe80
swi_ovf_count:
        .long   _mars_swdt_ovf_count
swi_clr_ovf:
        .word   0xa53e                  /* A5 = sel WTCSR, 3E = clr OVF, IT mode, timer enabled, clksel = Fs/4096 */

!-----------------------------------------------------------------------
! Secondary RESET IRQ handler
!-----------------------------------------------------------------------

sec_vres_irq:
        mov.l   svri_mars_adapter,r1
        mov.w   r0,@(0x14,r1)           /* clear VRES IRQ */

        mov     #0x0F,r0
        shll2   r0
        shll2   r0
        ldc     r0,sr                   /* disallow ints */

        mov.l   svri_sec_stk,r15
        mov.l   svri_sec_vres,r0
        jmp     @r0
        nop

        .align  2
svri_mars_adapter:
        .long   0x20004000
svri_sec_stk:
        .long   sec_stack               /* Cold Start SP */
svri_sec_vres:
        .long   sec_reset


!-----------------------------------------------------------------------
!-----------------------------------------------------------------------
! Support Functions
!-----------------------------------------------------------------------
!-----------------------------------------------------------------------

! void fast_memcpy(int *dst, int *src, int len);
! Fast memcpy function - copies longs, runs from sdram for speed
! On entry: r4 = dst, r5 = src, r6 = len (in longs)

        .align  4
        .global _fast_memcpy
_fast_memcpy:
        mov.l   @r5+,r3
        mov.l   r3,@r4
        dt      r6
        bf/s    _fast_memcpy
        add     #4,r4
        rts
        nop


! Fast memset function - sets long values, runs from sdram for speed
! On entry: r4 = dst, r5 = value, r6 = len (in longs)

        .align  4
        .global _fast_memset
_fast_memset:
        mov.l   r5,@r4
        dt      r6
        bf/s    _fast_memset
        add     #4,r4
        rts
        nop


! void CacheControl(int mode);
! Cache control function
! On entry: r4 = cache mode => 0x10 = CP, 0x08 = TW, 0x01 = CE

        .align  4
        .global _CacheControl
_CacheControl:
        mov.l   _sh2_cctl,r0
        mov.b   r4,@r0
        rts
        nop

        .align  2

_sh2_cctl:
        .long   0xFFFFFE92


! int SetSH2SR(int level);
! On entry: r4 = new irq level
! On exit:  r0 = old irq level
        .align  4
        .global _SetSH2SR
_SetSH2SR:
        stc     sr,r1
        mov     #0x0F,r0
        shll2   r0
        shll2   r0
        and     r0,r1                   /* just the irq mask */
        shlr2   r1
        shlr2   r1
        not     r0,r0
        stc     sr,r2
        and     r0,r2
        shll2   r4
        shll2   r4
        or      r4,r2
        ldc     r2,sr
        rts
        mov     r1,r0

!-----------------------------------------------------------------------
! Primary and Secondary RESET code
!-----------------------------------------------------------------------

        .align  2

        .text

pri_reset:
        ! do any primary SH2 specific reset code here

        mov.l   sec_st,r0
        mov.l   sec_ok,r1
0:
        mov.l   @r0,r2
        nop
        nop
        cmp/eq  r1,r2
        bf      0b                      /* wait for secondary sh2 */

        ! recopy rom data to sdram
        mov.l   rom_header,r1
        mov.l   @r1,r2                  /* src relative to start of rom */
        mov.l   @(4,r1),r3              /* dst relative to start of sdram */
        mov.l   @(8,r1),r4              /* size (longword aligned) */
        mov.l   rom_start,r1
        add     r1,r2
        mov.l   sdram_start,r1
        add     r1,r3
        shlr2   r4                      /* number of longs */
        add     #-1,r4
1:
        mov.l   @r2+,r0
        mov.l   r0,@r3
        add     #4,r3
        dt      r4
        bf      1b

        mov.l   pri_st,r0
        mov.l   pri_ok,r1
        mov.l   r1,@r0                  /* tell everyone reset complete */

        mov.l   pri_go,r0
        jmp     @r0
        nop

sec_reset:
        ! do any secondary SH2 specific reset code here

        mov.l   sec_st,r0
        mov.l   sec_ok,r1
        mov.l   r1,@r0                  /* tell primary to start reset */

        mov.l   pri_st,r0
        mov.l   pri_ok,r1
0:
        mov.l   @r0,r2
        nop
        nop
        cmp/eq  r1,r2
        bf      0b                      /* wait for primary to do the work */

        mov.l   sec_go,r0
        jmp     @r0
        nop

        .align  2
pri_st:
        .long   0x20004020
pri_ok:
        .ascii  "M_OK"
pri_go:
        .long   pri_start
rom_header:
        .long   0x220003D4
rom_start:
        .long   0x22000000
sdram_start:
        .long   0x26000000

sec_st:
        .long   0x20004024
sec_ok:
        .ascii  "S_OK"
sec_go:
        .long   sec_start


! this suppresses a warning in the linker about missing start()

        .global _start
_start:
