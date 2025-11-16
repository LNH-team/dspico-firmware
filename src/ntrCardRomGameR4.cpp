#include "common.h"
#include <stdio.h>
#include <string.h>
#include "sd/fatfs/ff.h"
#include "ntrCardRom.h"
#include "ntrCardRomGameNoScramble.h"

#ifdef ENABLE_R4_MODE

static FIL sRomFile;
static FIL sSaveFile;

static volatile bool sInitR4Rom = false;
static volatile u32 sR4RomFatEntryAddr = 0;
static volatile u32 sR4RomFetchAddr = 0;
static volatile u32 sR4CurRomBlockAddr = 0;
static volatile u32 sR4RomReadStat = 1;

static volatile bool sInitR4Save = false;
static volatile u32 sR4SaveFatEntryAddr = 0;
static volatile u32 sR4SaveFetchAddr = 0xFFFFFFFF;
static volatile u32 sR4CurSaveBlockAddr = 0xFFFFFFFF;

static volatile bool sSaveWriteBusy = true;
static volatile u32 sWriteSaveAddr = 0xFFFFFFFF;

static u8 sR4RomBlock[512];
static u8 sR4SaveBlock[512];

static u32 sR4RomBlockLargeAddr = 0xFFFFFFFF;
static u8 sR4RomBlockLarge[16384];//512];

static DWORD sClusterTab[16384];
static DWORD sSaveClusterTab[4096];

extern FATFS sFatFs;

extern "C" void __scratch_y("cpu0") ntrc_gameR4DummyCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    ntrc_writeWord(pio, 0);
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4GetCardInfoCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    ntrc_writeWord(pio, 0x1F4);
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4StartSaveReadCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    u32 addr = word << 8;
    u32 result = 1;
    if (addr == sR4CurSaveBlockAddr)
    {
        result = 0;
        sR4CurSaveBlockAddr = 0xFFFFFFFF;
    }
    else if (sR4SaveFetchAddr != addr)
    {
        sR4SaveFetchAddr = addr;
    }
    ntrc_writeWord(pio, result); //0=cmd0,1=cmd1,2=payload

    romEmu->wordIdx = 1;
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4GetSaveDataCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 512);
    ntrc_dmaToBus(sR4SaveBlock, 512);
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4GetSaveDataCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameNoScrambleCmd1(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4GetSaveStatCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    u32 sdStat = 1;
    if (!sSaveWriteBusy && sWriteSaveAddr == 0xFFFFFFFF)
    {
        sdStat = 0;
    }
    ntrc_writeWord(pio, sdStat);
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4SendMapCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);

    u64 cmd = (((u64)romEmu->cmd0) << 32) | word; 
    u32 addr = (cmd >> 24) & 0xFFFFFFFF;
    u32 result = 1;
    if (!(addr & 1))
    {
        //rom
        if (sR4RomFatEntryAddr == addr)
        {
            result = sInitR4Rom;
        }
        else
        {
            sR4RomFatEntryAddr = addr;
            sInitR4Rom = true;
        }
    }
    else
    {
        //save
        addr &= ~1;
        if (sR4SaveFatEntryAddr == addr)
        {
            result = sInitR4Save;
        }
        else
        {
            sR4SaveFatEntryAddr = addr;
            sInitR4Save = true;
        }
    }

    ntrc_writeWord(pio, result);

    ntrc_finishGameNoScrambleCmd1(romEmu);
}

static void __no_inline_not_in_flash_func(gameR4StartRomReadCmd0Cont)(ntr_rom_emu_t* romEmu)
{
    u32 addr = romEmu->cmd0 << 8;
    if (addr == sR4CurRomBlockAddr)
    {
        sR4CurRomBlockAddr = 0;
    }
    else if (addr != sR4CurRomBlockAddr && sR4RomFetchAddr != addr)
    {
        sR4RomFetchAddr = addr;
    }

    romEmu->wordIdx = 1;
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4StartRomReadCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    u32 addr = romEmu->cmd0 << 8;
    u32 stat = (addr == sR4CurRomBlockAddr) ? 0 : 1;
    ntrc_writeWord(pio, stat);
    gameR4StartRomReadCmd0Cont(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4StartSaveWriteCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginRead(pio, 512);
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

static void __scratch_y("cpu0") r4SaveWritePayloadComplete(ntr_rom_emu_t* romEmu)
{
    sWriteSaveAddr = romEmu->cmd0 << 8;
    sSaveWriteBusy = true;
}

extern "C" void __scratch_y("cpu0") ntrc_gameR4StartSaveWriteCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameNoScrambleCmd1WithReadPayload(romEmu, (u32*)sR4SaveBlock, 512, r4SaveWritePayloadComplete);
}

extern "C" void __scratch_y("cpu0") ntrc_gameNoScrambleReadPageCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 512);
#ifdef ENABLE_R4_MODE
    if (romEmu->r4Mode)
    {
        u32 address = romEmu->cmd0 << 8;
        ntrc_dmaToBus(&sR4RomBlockLarge[address & 0x3FFF], 512);
    }
#endif
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern "C" void __scratch_y("cpu0") ntrc_gameNoScrambleReadPageCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
#ifdef ENABLE_R4_MODE
    if (!romEmu->r4Mode)
#endif
    {
        //just send some garbage
        ntrc_dmaToBus(sR4RomBlockLarge, 512);
    }
    ntrc_finishGameNoScrambleCmd1(romEmu);
}

static void initFileFromFatEntry(FIL* file, u32 fatEntryAddr, bool save)
{
    while (!gSdCard.TryReadSectorsSync(sR4RomBlock, fatEntryAddr >> 9, 1));
    const u8* fatEntry = &sR4RomBlock[fatEntryAddr & 0x1FF];

    u32 firstCluster = *(const u16*)&fatEntry[26];
    firstCluster |= *(const u16*)&fatEntry[20] << 16;

    file->obj.fs = &sFatFs;
    file->obj.id = sFatFs.id;
    file->obj.sclust = firstCluster;
    file->obj.attr = 0;
    file->obj.objsize = *(const u32*)&fatEntry[28];
    file->dir_sect = fatEntryAddr >> 9;
    file->dir_ptr = &sFatFs.win[fatEntryAddr & 0x1FF];
    file->cltbl = 0;
    file->flag = save ? (FA_READ | FA_WRITE) : FA_READ;
    file->err = 0;
    file->sect = 0;
    file->fptr = 0;
}

extern "C" void __time_critical_func(ntrc_gameR4Update)(void)
{
    if (sInitR4Rom)
    {
        initFileFromFatEntry(&sRomFile, sR4RomFatEntryAddr, false);
        gNtrRomEmu.romSize = f_size(&sRomFile);

        sRomFile.cltbl = sClusterTab;
        sClusterTab[0] = 16384;
        while (f_lseek(&sRomFile, CREATE_LINKMAP) != FR_OK);

        gNtrRomEmu.r4Mode = true;
        sInitR4Rom = false;
    }

    if (sInitR4Save)
    {
        initFileFromFatEntry(&sSaveFile, sR4SaveFatEntryAddr, true);

        sSaveFile.cltbl = sSaveClusterTab;
        sSaveClusterTab[0] = 4096;
        while (f_lseek(&sSaveFile, CREATE_LINKMAP) != FR_OK);

        sInitR4Save = false;
    }

    if (sR4RomFetchAddr)
    {
        if ((sR4RomFetchAddr & ~0x3FFF) != sR4RomBlockLargeAddr)
        {
            while (f_lseek(&sRomFile, sR4RomFetchAddr & ~0x3FFF) != FR_OK);
            UINT br;
            while (f_read(&sRomFile, sR4RomBlockLarge, sizeof(sR4RomBlockLarge), &br) != FR_OK);
            sR4RomBlockLargeAddr = sR4RomFetchAddr & ~0x3FFF;
        }

        sR4CurRomBlockAddr = sR4RomFetchAddr;
        sR4RomFetchAddr = 0;
    }

    if (sR4SaveFetchAddr != 0xFFFFFFFF)
    {
        while (f_lseek(&sSaveFile, sR4SaveFetchAddr) != FR_OK);
        UINT br;
        while (f_read(&sSaveFile, sR4SaveBlock, 512, &br) != FR_OK);

        sR4CurSaveBlockAddr = sR4SaveFetchAddr;
        sR4SaveFetchAddr = 0xFFFFFFFF;
    }

    if (sSaveWriteBusy && sWriteSaveAddr != 0xFFFFFFFF)
    {
        while (f_lseek(&sSaveFile, sWriteSaveAddr) != FR_OK);
        UINT br;
        while (f_write(&sSaveFile, sR4SaveBlock, 512, &br) != FR_OK);
        while (f_sync(&sSaveFile) != FR_OK);

        sWriteSaveAddr = 0xFFFFFFFF;
        sSaveWriteBusy = false;
    }
}

extern "C" void ntrc_resetR4(void)
{
    gNtrRomEmu.r4Mode = false;
    sInitR4Rom = false;
    sR4RomFatEntryAddr = 0;
    sR4RomFetchAddr = 0;
    sR4CurRomBlockAddr = 0;
    sR4RomReadStat = 1;

    sInitR4Save = false;
    sR4SaveFatEntryAddr = 0;
    sR4SaveFetchAddr = 0xFFFFFFFF;
    sR4CurSaveBlockAddr = 0xFFFFFFFF;

    sSaveWriteBusy = true;
    sWriteSaveAddr = 0xFFFFFFFF;

    sR4RomBlockLargeAddr = 0xFFFFFFFF;
}

#endif
