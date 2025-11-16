#include "common.h"
#include "scrambler.h"

void __time_critical_func(scr_init)(scr_state_t* state, u64 seed0)
{
    u64 x = 0;
    for (int i = 0; i < 39; i++)
    {
        x = (x << 1) | (seed0 & 1);
        seed0 >>= 1;
    }
    state->x = x;
    state->y = 0b101000001101100111011001111000010011101ull;
}

__attribute__((optimize("O3")))
u32 __scratch_x("cpu1") scr_getNext32(scr_state_t* state)
{
    u64 x = state->x;
    u32 xm1 = x;
    ((vu32*)&state->x)[1] = xm1;
    __compiler_memory_barrier();

    u32 xlo = (u32)(x >> 7);
    u32 tmp = xm1 << 6;
    xlo ^= tmp;
    tmp <<= 1;
    xlo ^= tmp;
    xlo ^= tmp << 12;
    tmp = xlo >> 25;
    xlo ^= tmp;
    xlo ^= tmp << 12;
    xlo ^= xlo >> 26;
    xlo ^= (xlo << 7) >> 20;

    ((vu32*)&state->x)[0] = xlo;
    __compiler_memory_barrier();
    
    u64 y = state->y;
    u32 ym1 = y;
    ((vu32*)&state->y)[1] = ym1;
    __compiler_memory_barrier();

    u32 ylo = (u32)(y >> 7);
    tmp = ym1 << 1;
    ylo ^= tmp;
    tmp <<= 5;
    ylo ^= tmp;
    ylo ^= tmp << 13;
    tmp = ylo >> 26;
    ylo ^= tmp;
    ylo ^= tmp << 13;
    ylo ^= ylo >> 31;
    ylo ^= (ylo << 6) >> 19;
 
    ((vu32*)&state->y)[0] = ylo;

    return __builtin_bswap32(((vu32*)&state->x)[0] ^ ylo);
}
