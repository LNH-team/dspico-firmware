.syntax unified
.section .scratch_y.cpu0, "ax"
.thumb

#define PIO0_BASE   0x50200000
#define PIO0_TXF0   (PIO0_BASE + 0x010)
#define PIO0_RXF0   (PIO0_BASE + 0x020)

.global ntrc_pioIrq
.type ntrc_pioIrq,%function
.thumb_func
ntrc_pioIrq:
    ldr r0,= gNtrRomEmu
    ldr r2,= PIO0_BASE
    ldr r3, [r0, #8] //wordIdx
    ldr r1, [r2, #(PIO0_RXF0 - PIO0_BASE)]
    cmp r3, #2
        bge payload
    lsls r3, r3, #2
    ldr r3, [r0, r3]
    bx r3

payload:
    ldr r2, [r0, #0x18] // readDataDestination
    rev r1, r1
    stmia r2!, {r1}
    str r2, [r0, #0x18] // readDataDestination
    ldr r2, [r0, #0x1C] // readDataLimit
    cmp r3, r2
        bhs payloadComplete // complete when wordIdx >= readDataLimit
    adds r3, r3, #1
    str r3, [r0, #8] // wordIdx++
    bx lr

payloadComplete:
    movs r3, #0
    ldr r2, [r0, #0x20] // readDataCompleteHandler
    str r3, [r0, #8] // wordIdx = 0
    str r3, [r0, #0x18] // readDataDestination = 0
    str r3, [r0, #0x1C] // readDataLimit = 0
    str r3, [r0, #0x20] // readDataCompleteHandler = 0
    bx r2

.balign 4
.pool

nop

.global ntrc_gameCmd0Handler
.type ntrc_gameCmd0Handler,%function
.thumb_func
ntrc_gameCmd0Handler:
    ldr r3, [r0, #0xC] //scrRingRPtr
    ldr r3, [r3]
    rev r3, r3
    eors r1, r1, r3
    str r1, [r0, #0x10] //cmd0
    lsrs r3, r1, #24
    cmp r3, #0xB7
        beq 1f
    cmp r3, #0xB8
        beq 2f
    ldr r3,= ntrc_gameCmd0Dummy
    bx r3

1:
    ldr r3,= ntrc_gameReadPageCmd0
    bx r3

2:
    ldr r3,= ntrc_gameReadIdCmd0
    bx r3

.balign 4
.pool

nop

.global ntrc_gameCmd1Handler
.type ntrc_gameCmd1Handler,%function
.thumb_func
ntrc_gameCmd1Handler:
    ldr r3, [r0, #0xC] //scrRingRPtr
    ldr r3, [r3]
    rev r3, r3
    eors r1, r1, r3
    str r1, [r0, #0x14] //cmd1
    ldrb r3, [r0, #0x13] //cmd0 >> 24
    cmp r3, #0xB7
        beq 1f
    cmp r3, #0xB8
        beq 2f
    cmp r3, #0xFC
        beq 3f
    ldr r3,= ntrc_gameCmd0Dummy
    bx r3

1:
    ldr r3,= ntrc_gameReadPageCmd1
    bx r3

2:
    ldr r3,= ntrc_gameCmd1Dummy4
    bx r3

3:
    ldr r3,= ntrc_gameDisableScrambleCmd1
    bx r3

.balign 4
.pool

lessThanB0Cmd0:
#ifdef ENABLE_R4_MODE
    cmp r1, #0
        beq r4Dummy0Cmd0
#endif
    ldr r3,= ntrc_gameNoScrambleCmd0Dummy
    bx r3

#ifdef ENABLE_R4_MODE
r4Dummy0Cmd0:
    ldr r3,= ntrc_gameR4DummyCmd0
    bx r3
#endif

nop

.global ntrc_gameNoScrambleCmd0Handler
.type ntrc_gameNoScrambleCmd0Handler,%function
.thumb_func
ntrc_gameNoScrambleCmd0Handler:
    str r1, [r0, #0x10] //cmd0
    lsrs r3, r1, #24
    subs r3, r3, #0xB0
        bmi lessThanB0Cmd0
    lsls r3, r3, #2
    add r3, r3, pc
    ldr r3, [r3, #(gGameNoScrambleCmd0HandlerTable - . - 2)]
    bx r3

.balign 4
.pool

//from 0xB0 to 0xFF
.global gGameNoScrambleCmd0HandlerTable
gGameNoScrambleCmd0HandlerTable:
#ifdef ENABLE_R4_MODE
    .word ntrc_gameR4GetCardInfoCmd0 //B0
#else
    .word ntrc_gameNoScrambleCmd0Dummy //B0
#endif
    .word ntrc_gameNoScrambleCmd0Dummy //B1
#ifdef ENABLE_R4_MODE
    .word ntrc_gameR4StartSaveReadCmd0 //B2
    .word ntrc_gameR4GetSaveDataCmd0 //B3
    .word ntrc_gameNoScrambleCmd0Dummy //B4
#else
    .word ntrc_gameNoScrambleCmd0Dummy //B2
    .word ntrc_gameNoScrambleCmd0Dummy //B3
    .word ntrc_gameNoScrambleCmd0Dummy //B4
#endif
    .word ntrc_gameNoScrambleCmd0Dummy //B5
#ifdef ENABLE_R4_MODE
    .word ntrc_gameR4StartRomReadCmd0 //B6
#else
    .word ntrc_gameNoScrambleCmd0Dummy //B6
#endif
    .word ntrc_gameNoScrambleReadPageCmd0 //B7
    .word ntrc_gameNoScrambleReadIdCmd0 //B8
#ifdef ENABLE_R4_MODE
    .word ntrc_gameR4StartSdReadCmd0 //B9
    .word ntrc_gameNoScrambleCmd0Dummy //BA
    .word ntrc_gameR4StartSdWriteCmd0 //BB
    .word ntrc_gameR4GetSdWriteStatCmd0 //BC
    .word ntrc_gameR4StartSaveWriteCmd0 //BD
    .word ntrc_gameR4GetSaveStatCmd0 //BE
#else
    .word ntrc_gameNoScrambleCmd0Dummy //B9
    .word ntrc_gameNoScrambleCmd0Dummy //BA
    .word ntrc_gameNoScrambleCmd0Dummy //BB
    .word ntrc_gameNoScrambleCmd0Dummy //BC
    .word ntrc_gameNoScrambleCmd0Dummy //BD
    .word ntrc_gameNoScrambleCmd0Dummy //BE
#endif
    .word ntrc_gameNoScrambleCmd0Dummy //BF
    //C0-DF
.rept 32
    .word ntrc_gameNoScrambleCmd0Dummy
.endr
    .word ntrc_gameNoScrambleCmd0Dummy //E0
    .word ntrc_gameNoScrambleCmd0Dummy //E1
    .word ntrc_gameNoScrambleCmd0Dummy //E2
    .word ntrc_gameNoScrambleCmd0Dummy //E3
    .word ntrc_gameGetSdStatCmd0 //E4
    .word ntrc_gameGetSdDataCmd0 //E5
    .word ntrc_gameNoScrambleCmd0Dummy //E6
    .word ntrc_gameNoScrambleCmd0Dummy //E7
    .word ntrc_gameNoScrambleCmd0Dummy //E8
    .word ntrc_gameWriteUsbDataCmd0 //E9
    .word ntrc_gameReadUsbDataCmd0 //EA
    .word ntrc_gameUsbGetEventCmd0 //EB
    .word ntrc_gameNoScrambleCmd0Dummy //EC
    .word ntrc_gameNoScrambleCmd0Dummy //ED
    .word ntrc_gameNoScrambleCmd0Dummy //EE
    .word ntrc_gameNoScrambleCmd0Dummy //EF
    .word ntrc_gameNoScrambleCmd0Dummy //F0
    .word ntrc_gameNoScrambleCmd0Dummy //F1
    .word ntrc_gameNoScrambleCmd0Dummy //F2
    .word ntrc_gameNoScrambleCmd0Dummy //F3
    .word ntrc_gameNoScrambleCmd0Dummy //F4
    .word ntrc_gameNoScrambleCmd0Dummy //F5
    .word ntrc_gameWriteSdDataCmd0 //F6
    .word ntrc_gameNoScrambleCmd0Dummy //F7
    .word ntrc_gameNoScrambleCmd0Dummy //F8
    .word ntrc_gameNoScrambleCmd0Dummy //F9
    .word ntrc_gameNoScrambleCmd0Dummy //FA
    .word ntrc_gameNoScrambleCmd0Dummy //FB
    .word ntrc_gameNoScrambleCmd0Dummy //FC
    .word ntrc_gameNoScrambleCmd0Dummy //FD
    .word ntrc_gameNoScrambleCmd0Dummy //FE
    .word ntrc_gameNoScrambleCmd0Dummy //FF

lessThanB0Cmd1:
#ifdef ENABLE_R4_MODE
    ldr r3, [r0, #0x10] //cmd0
    cmp r3, #0
        beq r4Dummy0Cmd1
#endif
    ldr r3,= ntrc_gameNoScrambleCmd1Unknown
    bx r3

#ifdef ENABLE_R4_MODE
r4Dummy0Cmd1:
    ldr r3,= ntrc_gameNoScrambleCmd1Dummy4
    bx r3
#endif

.balign 4
.pool

nop

.global ntrc_gameNoScrambleCmd1Handler
.type ntrc_gameNoScrambleCmd1Handler,%function
.thumb_func
ntrc_gameNoScrambleCmd1Handler:
    str r1, [r0, #0x14] //cmd1
    ldrb r3, [r0, #0x13] //cmd0 >> 24
    subs r3, r3, #0xB0
        bmi lessThanB0Cmd1
    lsls r3, r3, #2
    add r3, r3, pc
    ldr r3, [r3, #(gGameNoScrambleCmd1HandlerTable - . - 2)]
    bx r3

.balign 4
.pool

//from 0xB0 to 0xFF
.global gGameNoScrambleCmd1HandlerTable
gGameNoScrambleCmd1HandlerTable:
#ifdef ENABLE_R4_MODE
    .word ntrc_gameNoScrambleCmd1Dummy4 //B0
#else
    .word ntrc_gameNoScrambleCmd1Unknown //B0
#endif
    .word ntrc_gameNoScrambleCmd1Unknown //B1
#ifdef ENABLE_R4_MODE
    .word ntrc_gameNoScrambleCmd1Dummy4 //B2
    .word ntrc_gameR4GetSaveDataCmd1 //B3
    .word ntrc_gameR4SendMapCmd1 //B4
#else
    .word ntrc_gameNoScrambleCmd1Unknown //B2
    .word ntrc_gameNoScrambleCmd1Unknown //B3
    .word ntrc_gameNoScrambleCmd1Unknown //B4
#endif
    .word ntrc_gameNoScrambleCmd1Unknown //B5
#ifdef ENABLE_R4_MODE
    .word ntrc_gameNoScrambleCmd1Dummy4 //B6
#else
    .word ntrc_gameNoScrambleCmd1Unknown //B6
#endif
    .word ntrc_gameNoScrambleReadPageCmd1 //B7
    .word ntrc_gameNoScrambleCmd1Dummy4 //B8
#ifdef ENABLE_R4_MODE
    .word ntrc_gameNoScrambleCmd1Dummy4 //B9
    .word ntrc_gameR4GetSdDataCmd1 //BA
    .word ntrc_gameR4StartSdWriteCmd1 //BB
    .word ntrc_gameNoScrambleCmd1Dummy4 //BC
    .word ntrc_gameR4StartSaveWriteCmd1 //BD
    .word ntrc_gameNoScrambleCmd1Dummy4 //BE
#else
    .word ntrc_gameNoScrambleCmd1Unknown //B9
    .word ntrc_gameNoScrambleCmd1Unknown //BA
    .word ntrc_gameNoScrambleCmd1Unknown //BB
    .word ntrc_gameNoScrambleCmd1Unknown //BC
    .word ntrc_gameNoScrambleCmd1Unknown //BD
    .word ntrc_gameNoScrambleCmd1Unknown //BE
#endif
    .word ntrc_gameNoScrambleCmd1Unknown //BF
    //C0-DF
.rept 32
    .word ntrc_gameNoScrambleCmd1Unknown
.endr
    .word ntrc_gameNoScrambleCmd1Unknown //E0
    .word ntrc_gameNoScrambleCmd1Unknown //E1
    .word ntrc_gameNoScrambleCmd1Unknown //E2
    .word ntrc_gameReqSdReadCmd1 //E3
    .word ntrc_gameNoScrambleCmd1Dummy4 //E4
    .word ntrc_gameGetSdDataCmd1 //E5
    .word ntrc_gameNoScrambleCmd1Unknown //E6
    .word ntrc_gameNoScrambleCmd1Unknown //E7
    .word ntrc_gameReqUsbCommandCmd1 //E8
    .word ntrc_gameWriteUsbDataCmd1 //E9
    .word ntrc_gameReadUsbDataCmd1 //EA
    .word ntrc_gameUsbGetEventCmd1 //EB
    .word ntrc_gameNoScrambleCmd1Unknown //EC
    .word ntrc_gameNoScrambleCmd1Unknown //ED
    .word ntrc_gameNoScrambleCmd1Unknown //EE
    .word ntrc_gameNoScrambleCmd1Unknown //EF
    .word ntrc_gameNoScrambleCmd1Unknown //F0
    .word ntrc_gameNoScrambleCmd1Unknown //F1
    .word ntrc_gameNoScrambleCmd1Unknown //F2
    .word ntrc_gameNoScrambleCmd1Unknown //F3
    .word ntrc_gameNoScrambleCmd1Unknown //F4
    .word ntrc_gameNoScrambleCmd1Unknown //F5
    .word ntrc_gameWriteSdDataCmd1 //F6
    .word ntrc_gameNoScrambleCmd1Unknown //F7
    .word ntrc_gameNoScrambleCmd1Unknown //F8
    .word ntrc_gameNoScrambleCmd1Unknown //F9
    .word ntrc_gameNoScrambleCmd1Unknown //FA
    .word ntrc_gameNoScrambleCmd1Unknown //FB
    .word ntrc_gameNoScrambleCmd1Unknown //FC
    .word ntrc_gameNoScrambleCmd1Unknown //FD
    .word ntrc_gameNoScrambleCmd1Unknown //FE
    .word ntrc_gameNoScrambleCmd1Unknown //FF

.end
