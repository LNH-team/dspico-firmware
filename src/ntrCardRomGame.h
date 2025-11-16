#pragma once
#include "common.h"
#include "ntrCardRom.h"
#include "scramblerRing.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Finishes handling the first command word in game mode.
/// @param romEmu The rom emulator context.
static inline void ntrc_finishGameCmd0(ntr_rom_emu_t* romEmu)
{
    scr_advanceRing(romEmu, 1); //cmd
    romEmu->wordIdx = 1;
}

/// @brief Finishes handling the second command word in game mode.
/// @param romEmu The rom emulator context.
/// @param writePayloadWords The number of response words for this command (DSpico -> DS).
static inline void ntrc_finishGameCmd1(ntr_rom_emu_t* romEmu, int writePayloadWords)
{
    scr_advanceRing(romEmu, 1 + writePayloadWords); //cmd+payload
    romEmu->wordIdx = 0;
}

#ifdef __cplusplus
}
#endif