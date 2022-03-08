
        .text

        .global Sub_Start
Sub_Start:

| Standard MegaCD Sub-CPU Program Header (copied to 0x6000)

SPHeader:
        .asciz  "MAIN-SUBCPU"
        .word   0x0001,0x0000
        .long   0x00000000
        .long   0x00000000
        .long   SPHeaderOffsets-SPHeader
        .long   0x00000000

SPHeaderOffsets:
        .word   SPInit-SPHeaderOffsets
        .word   SPMain-SPHeaderOffsets
        .word   SPInt2-SPHeaderOffsets
        .word   SPNull-SPHeaderOffsets
        .word   0x0000

| Sub-CPU Program Initialization (VBlank not enabled yet)

SPInit:
        move.b  #'I,0x800F.w            /* sub comm port = INITIALIZING */
        andi.b  #0xE2,0x8003.w          /* Priority Mode = off, 2M mode, Sub-CPU has access */
        rts

| Sub-CPU Program Main Entry Point (VBlank now enabled)

SPMain:
        move.w  #0x0081,d0              /* CDBSTAT */
        jsr     0x5F22.w                /* call CDBIOS function */
        move.w  0(a0),d0                /* BIOS status word */
        bmi.b   1f                      /* not ready */
        lsr.w   #8,d0
        cmpi.b  #0x40,d0
        beq.b   9f                      /* open */
        cmpi.b  #0x10,d0
        beq.b   9f                      /* no disc */
1:
| Initialize Drive
        lea     drive_init_parms(pc),a0
        move.w  #0x0010,d0              /* DRVINIT */
        jsr     0x5F22.w                /* call CDBIOS function */

        move.w  #0x0089,d0              /* CDCSTOP - stop reading data */
        jsr     0x5F22.w                /* call CDBIOS function */
9:
        move.b  #0,0x800F.w             /* sub comm port = READY */

| wait for command in main comm port
WaitCmd:
        tst.b   0x800E.w
        beq.b   WaitCmd
        cmpi.b  #'D,0x800E.w
        beq     GetDiscInfo
        cmpi.b  #'T,0x800E.w
        beq     GetTrackInfo
        cmpi.b  #'P,0x800E.w
        beq     PlayTrack
        cmpi.b  #'S,0x800E.w
        beq     StopPlaying
        cmpi.b  #'Z,0x800E.w
        beq     PauseResume
        cmpi.b  #'C,0x800E.w
        beq     CheckDisc
        move.b  #'E,0x800F.w            /* sub comm port = ERROR */
WaitAck:
        tst.b   0x800E.w
        bne.b   WaitAck                 /* wait for result acknowledged */
        move.b  #0,0x800F.w             /* sub comm port = READY */
        bra.b   WaitCmd

GetDiscInfo:
        move.w  #0x0081,d0              /* CDBSTAT */
        jsr     0x5F22.w                /* call CDBIOS function */
        move.w  0(a0),0x8020.w          /* BIOS status word */
        move.w  16(a0),0x8022.w         /* First song number, Last song number */
        move.w  18(a0),0x8024.w         /* Drive version, Flag */

        move.b  #'D,0x800F.w            /* sub comm port = DONE */
        bra     WaitAck

GetTrackInfo:
        move.w  0x8010.w,d1             /* track number */
        move.w  #0x0083,d0              /* CDBTOCREAD */
        jsr     0x5F22.w                /* call CDBIOS function */
        move.l  d0,0x8020.w             /* MMSSFFTN */
        move.b  d1,0x8024.w             /* track type */

        move.b  #'D,0x800F.w            /* sub comm port = DONE */
        bra     WaitAck

PlayTrack:
        move.w  #0x0002,d0              /* MSCSTOP - stop playing */
        jsr     0x5F22.w                /* call CDBIOS function */

        move.w  0x8010.w,d1             /* track number */
        move.w  #0x0011,d0              /* MSCPLAY - play from track on */
        move.b  0x8012.w,d2             /* flag */
        bmi.b   2f
        beq.b   1f
        move.w  #0x0013,d0              /* MSCPLAYR - play with repeat */
        bra.b   2f
1:
        move.w  #0x0012,d0              /* MSCPLAY1 - play once */
2:
        lea     track_number(pc),a0
        move.w  d1,(a0)
        jsr     0x5F22.w                /* call CDBIOS function */

        move.b  #'D,0x800F.w            /* sub comm port = DONE */
        bra     WaitAck

StopPlaying:
        move.w  #0x0002,d0              /* MSCSTOP - stop playing */
        jsr     0x5F22.w                /* call CDBIOS function */

        move.b  #'D,0x800F.w            /* sub comm port = DONE */
        bra     WaitAck

PauseResume:
        move.w  #0x0081,d0              /* CDBSTAT */
        jsr     0x5F22.w                /* call CDBIOS function */
        move.b  (a0),d0
        cmpi.b  #1,d0
        beq.b   1f                      /* playing - pause playback */
        cmpi.b  #5,d0
        beq.b   2f                      /* paused - resume playback */

        move.b  #'E,0x800F.w            /* sub comm port = ERROR */
        bra     WaitAck
1:
        move.w  #0x0003,d0              /* MSCPAUSEON - pause playback */
        jsr     0x5F22.w                /* call CDBIOS function */

        move.b  #'D,0x800F.w            /* sub comm port = DONE */
        bra     WaitAck
2:
        move.w  #0x0004,d0              /* MSCPAUSEOFF - resume playback */
        jsr     0x5F22.w                /* call CDBIOS function */

        move.b  #'D,0x800F.w            /* sub comm port = DONE */
        bra     WaitAck

CheckDisc:
        lea     drive_init_parms(pc),a0
        move.w  #0x0010,d0              /* DRVINIT */
        jsr     0x5F22.w                /* call CDBIOS function */

        move.w  #0x0089,d0              /* CDCSTOP - stop reading data */
        jsr     0x5F22.w                /* call CDBIOS function */

        move.b  #'D,0x800F.w            /* sub comm port = DONE */
        bra     WaitAck


| Sub-CPU Program VBlank (INT02) Service Handler

SPInt2:
        rts

| Sub-CPU program Reserved Function

SPNull:
        rts


| Sub-CPU variables

        .align  2
drive_init_parms:
        .byte   0x01, 0xFF              /* first track (1), last track (all) */

track_number:
        .word   0


        .global Sub_End
Sub_End:
