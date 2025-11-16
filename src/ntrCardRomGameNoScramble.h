#pragma once
#include "common.h"
#include "ntrCardRom.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Finishes handling the first command word in unscrambled game mode.
/// @param romEmu The rom emulator context.
static inline void ntrc_finishGameNoScrambleCmd0(ntr_rom_emu_t* romEmu)
{
    romEmu->wordIdx = 1;
}

/// @brief Finishes handling the second command word in unscrambled game mode.
/// @param romEmu The rom emulator context.
static inline void ntrc_finishGameNoScrambleCmd1(ntr_rom_emu_t* romEmu)
{
    romEmu->wordIdx = 0;
}

/// @brief Finishes handling the second command word in unscrambled game mode with a read payload (DS -> DSpico).
/// @param romEmu The rom emulator context.
/// @param payloadDestination Destination buffer for the payload data.
/// @param payloadLength The number of payload bytes (must be a multiple of 4).
/// @param payloadCompleteCallback Callback function to be called when reading the payload is complete.
static inline void ntrc_finishGameNoScrambleCmd1WithReadPayload(ntr_rom_emu_t* romEmu, u32* payloadDestination,
    u32 payloadLength, ntrc_read_data_complete_handler_t payloadCompleteCallback)
{
    romEmu->readDataDestination = payloadDestination;
    romEmu->readDataLimit = (payloadLength >> 2) + 1;
    romEmu->readDataCompleteHandler = payloadCompleteCallback;
    romEmu->wordIdx = 2;
}

#ifdef __cplusplus
}
#endif