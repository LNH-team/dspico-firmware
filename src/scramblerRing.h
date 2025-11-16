#pragma once
#include "common.h"
#include "pico/multicore.h"
#include "ntrCardRom.h"
#include "scrambler.h"

#define SCR_RING_SIZE   1024

#define gScramblerRing  ((u32*)USBCTRL_DPRAM_BASE)

#define gScramblerRingRPtr  (*(vu32*)&gNtrRomEmu.scrRingRPtr)

#define SCR_RING_WRAP(x)    ((u32*)((u32)(x) & ~0xF000))

extern u32* volatile gScramblerRingWPtr;
extern scr_state_t gScramblerState;
extern volatile bool gComputeScrambler;

extern u8 gScrambleBuffers[2][512];
extern volatile int gFreeScrambleBuf;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Advances the scrambler ring by the given number of \p words.
/// @param romEmu The rom emulator context.
/// @param words The number of words to advance the scrambler ring by.
static inline void scr_advanceRing(ntr_rom_emu_t* romEmu, int words)
{
    romEmu->scrRingRPtr = SCR_RING_WRAP(romEmu->scrRingRPtr + words);
    __sev();
}

#ifdef __cplusplus
}
#endif
