#include "common.h"
#include <stdio.h>
#include "r4.h"
#include "ntrCardRom.h"
#include "ntrCardRomGameNoScramble.h"

static u8 sSdSectorBuf[1024];
static u32 sCurSdSector = 0xFFFFFFFF;
static u32 sSdSectorBuffersSectors[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
static u32 sBufferIndex = 0;
static u32 sReadSector;
static bool sReadBusy = false;
static bool sWriteBusy = false;
static bool sNextWriteBlockQueued = false;
static bool sNextWriteIsLast = false;
static u32 sNextWriteSector = 0xFFFFFFFF;

extern "C" void __scratch_y("cpu0") ntrc_gameReqSdReadCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_noPayload(pio);
    sCurSdSector = 0xFFFFFFFF;
    sReadSector = word;
    if (sSdSectorBuffersSectors[sBufferIndex] != word)
    {
        sSdSectorBuffersSectors[0] = 0xFFFFFFFF;
        sSdSectorBuffersSectors[1] = 0xFFFFFFFF;
        sBufferIndex = 0;
        if (!gSdCard.TryBeginReadSectors(&sSdSectorBuf[0], sReadSector, 1))
        {
            __breakpoint();
        }
        sReadBusy = true;
    }
    ntrc_finishGameNoScrambleCmd1(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameGetSdStatCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);

    bool sdReady;
    if (sWriteBusy)
    {
        sdReady = gSdCard.IsReady();
        if (sdReady && !sNextWriteBlockQueued)
        {
            sWriteBusy = false;
        }
    }
    else
    {
        if (sSdSectorBuffersSectors[sBufferIndex] == sReadSector)
        {
            sdReady = true;
        }
        else if (sReadBusy && gSdCard.IsReady())
        {
            sSdSectorBuffersSectors[sBufferIndex] = sReadSector;
            sdReady = true;
            sReadBusy = false;
        }
        else
        {
            sdReady = false;
        }
    }

    ntrc_writeWord(pio, sdReady ? 1 : 0);
    ntrc_finishGameNoScrambleCmd0(romEmu);

    if (sWriteBusy && sdReady && sNextWriteBlockQueued)
    {
        if (!gSdCard.TryBeginWriteSectors(&sSdSectorBuf[sBufferIndex * 512], sNextWriteSector, 1, !sNextWriteIsLast))
        {
            __breakpoint();
        }

        sNextWriteBlockQueued = false;
        sNextWriteIsLast = false;
        sNextWriteSector = 0xFFFFFFFF;
    }
}

extern "C" void __scratch_y("cpu0") ntrc_gameGetSdSectCntCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    ntrc_writeWord(pio, gSdCard.GetNumSectors());
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameGetSdDataCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 512);

    // without scrambling to save time
    ntrc_dmaToBus(&sSdSectorBuf[sBufferIndex * 512], 512);
    sReadSector++;

    sBufferIndex = 1 - sBufferIndex;

    if (!sReadBusy)
    {
        if (!gSdCard.TryBeginReadSectors(&sSdSectorBuf[sBufferIndex * 512], sReadSector, 1))
        {
            __breakpoint();
        }
        sReadBusy = true;
    }

    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameGetSdDataCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    // we still need to advance the state
    ntrc_finishGameNoScrambleCmd1(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameWriteSdDataCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    if ((word & ~WRITE_SD_DATA_FLAGS_MASK) != NTR_CMD_ID_GAME_WRITE_SD_DATA)
    {
        ntrc_gameCmd0Dummy(romEmu, word, pio);
        return;
    }

    ntrc_beginRead(pio, 512);
    sCurSdSector = 0xFFFFFFFF;
    sSdSectorBuffersSectors[0] = 0xFFFFFFFF;
    sSdSectorBuffersSectors[1] = 0xFFFFFFFF;

    ntrc_finishGameNoScrambleCmd0(romEmu);
}

static void __scratch_y("cpu0") sdWritePayloadComplete(ntr_rom_emu_t* romEmu)
{
    bool isFirst = (romEmu->cmd0 & WRITE_SD_DATA_IS_FIRST_FLAG) != 0;
    bool isLast = (romEmu->cmd0 & WRITE_SD_DATA_IS_LAST_FLAG) != 0;
    if (isFirst)
    {
        if (!gSdCard.TryBeginWriteSectors(sSdSectorBuf, romEmu->cmd1, 1, !isLast))
        {
            __breakpoint();
        }
        sWriteBusy = true;
    }
    else
    {
        sNextWriteBlockQueued = true;
        sNextWriteIsLast = isLast;
        sNextWriteSector = romEmu->cmd1;
    }
}

extern "C" void __scratch_y("cpu0") ntrc_gameWriteSdDataCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    if ((romEmu->cmd0 & ~WRITE_SD_DATA_FLAGS_MASK) != NTR_CMD_ID_GAME_WRITE_SD_DATA)
    {
        ntrc_gameCmd1Unknown(romEmu, word, pio);
        return;
    }

    bool isFirst = (romEmu->cmd0 & WRITE_SD_DATA_IS_FIRST_FLAG) != 0;
    if (isFirst)
    {
        sNextWriteBlockQueued = false;
        sNextWriteIsLast = false;
        sNextWriteSector = 0xFFFFFFFF;
        sBufferIndex = 0;
    }
    else
    {
        sBufferIndex = 1 - sBufferIndex;
    }

    ntrc_finishGameNoScrambleCmd1WithReadPayload(romEmu,
        (u32*)&sSdSectorBuf[sBufferIndex * 512], 512, sdWritePayloadComplete);
}

#ifdef ENABLE_R4_MODE

extern "C" void __scratch_y("cpu0") ntrc_gameR4StartSdReadCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    u32 sector = (romEmu->cmd0 << 8) >> 9;
    u32 result = 0x1F4;
    if (gSdCard.IsReady())
    {
        if (sector == sCurSdSector)
        {
            result = 0;
        }
        else
        {
            if (!gSdCard.TryBeginReadSectors(sSdSectorBuf, sector, 1))
            {
                __breakpoint();
            }
            sCurSdSector = sector;
            sSdSectorBuffersSectors[0] = 0xFFFFFFFF;
            sSdSectorBuffersSectors[1] = 0xFFFFFFFF;
        }
    }
    ntrc_writeWord(pio, result);
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4GetSdDataCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 512);
    ntrc_dmaToBus(sSdSectorBuf, 512);
    romEmu->wordIdx = 0;
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4StartSdWriteCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginRead(pio, 512);
    sCurSdSector = 0xFFFFFFFF;
    sSdSectorBuffersSectors[0] = 0xFFFFFFFF;
    sSdSectorBuffersSectors[1] = 0xFFFFFFFF;
    sNextWriteBlockQueued = false;
    sNextWriteIsLast = false;
    sNextWriteSector = 0xFFFFFFFF;
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

static void __scratch_y("cpu0") r4SdWritePayloadComplete(ntr_rom_emu_t* romEmu)
{
    if (__builtin_expect(!gSdCard.TryBeginWriteSectors(sSdSectorBuf, (romEmu->cmd0 << 8) >> 9, 1, false), false))
    {
        __breakpoint();
    }
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4StartSdWriteCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameNoScrambleCmd1WithReadPayload(romEmu, (u32*)sSdSectorBuf, 512, r4SdWritePayloadComplete);
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4GetSdWriteStatCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    u32 sdStat = 1;
    if (gSdCard.IsReady())
        sdStat = 0;
    ntrc_writeWord(pio, sdStat);
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

#endif