.syntax unified
.section .data
.thumb

#define PIO0_BASE   0x50200000
#define PIO0_FSTAT  (PIO0_BASE + 0x004)
#define PIO0_TXF0   (PIO0_BASE + 0x010)

//r0 = srcA
//r1 = srcB
//r2 = dst
//r3 = length (multiple of 8)
.global applyXor
.type applyXor, %function
applyXor:
    push {r4-r7}

1:
    ldmia r0!, {r4,r5}
    ldmia r1!, {r6,r7}
    eors r4, r6
    eors r5, r7
    stmia r2!, {r4,r5}
    subs r3, #8
    bne 1b

    pop {r4-r7}
    bx lr

//r0 = srcA, r1 = srcB
@ .global xorToPio512
@ xorToPio512:
@     push {r4-r7}
@     ldr r2,= PIO0_FSTAT
@     ldr r3,= #(1 << 16)

@     ldr 

@     pop {r4-r7}
@     bx lr


.pool
.end
