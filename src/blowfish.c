#include "common.h"
#include "blowfish.h"

void bf_init(bf_context_t* context, const u32* pTable, const bf_sboxes_t* sBoxes)
{
    context->pTable = pTable;
    context->sBoxes = sBoxes;
}

u64 __time_critical_func(bf_decrypt)(const bf_context_t* context, u64 block)
{
    const u32* pTable = context->pTable;
    const bf_sboxes_t* sBoxes = context->sBoxes;
    u32 y = block;
    u32 x = block >> 32;
    for (int i = 0x11; i >= 2; i--)
    {
        u32 z = pTable[i] ^ x;
        u32 a = sBoxes->sbox0[z >> 24];
        u32 b = sBoxes->sbox1[(u8)(z >> 16)];
        u32 c = sBoxes->sbox2[(u8)(z >> 8)];
        u32 d = sBoxes->sbox3[(u8)z];
        x = (d + (c ^ (b + a))) ^ y;
        y = z;
    }

    return (x ^ pTable[1]) | ((u64) (y ^ pTable[0]) << 32);
}
