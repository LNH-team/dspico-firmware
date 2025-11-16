#include "common.h"
#include <stdio.h>
#include "xor.h"
#include "romData.h"
#include "scramblerRing.h"
#include "ntrCardRom.h"
#include "ntrCardRomGameNoScramble.h"
#include "ntrCardRomGame.h"

void ntrc_gameCmd1Unknown(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_noPayload(pio);
    ntrc_finishGameCmd1(romEmu, 0);

    //do not receive further commands until card reset
    irq_set_enabled(PIO0_IRQ_0, false);

    puts("Unknown game command");
    printf("%08X%08X\n", romEmu->cmd0, romEmu->cmd1);
}

void __time_critical_func(ntrc_gameCmd0Dummy)(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameCmd0(romEmu);
}

void __time_critical_func(ntrc_gameCmd1Dummy0)(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameCmd1(romEmu, 0);
}

void __time_critical_func(ntrc_gameCmd1Dummy4)(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameCmd1(romEmu, 1);
}

void __scratch_y("cpu0") ntrc_gameReadPageCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 512);
    ntrc_finishGameCmd0(romEmu);
}

void __scratch_y("cpu0") ntrc_gameReadPageCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    u32 address = romEmu->cmd0 << 8;

    if (address + 512 <= romEmu->romSize)
    {
        u32 rPtr = gScramblerRingRPtr;
        u32* outPtr = (u32*)&gScrambleBuffers[gFreeScrambleBuf][0];
        u32* inPtr = (u32*)&romEmu->romData[address];

        applyXor(inPtr, romEmu->scrRingRPtr + 1, outPtr, 512);
    }
    else
    {
        //just send some garbage
    }

    ntrc_dmaToBus(&gScrambleBuffers[gFreeScrambleBuf][0], 512);
    gFreeScrambleBuf = 1 - gFreeScrambleBuf;
    ntrc_finishGameCmd1(romEmu, 128);
}

void __scratch_y("cpu0") ntrc_gameReadIdCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    ntrc_writeWord(pio, romEmu->cardId ^ romEmu->scrRingRPtr[2]); //0=cmd0,1=cmd1,2=payload
    ntrc_finishGameCmd0(romEmu);
}

void __scratch_y("cpu0") ntrc_gameDisableScrambleCmd1(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_setGameNoScrambleMode();
    ntrc_noPayload(pio);
    ntrc_finishGameCmd1(romEmu, 0);
}

extern void ntrc_gameCmd0Handler(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);
extern void ntrc_gameCmd1Handler(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);

void __time_critical_func(ntrc_setGameMode)(void)
{
    gNtrRomEmu.cmd0Handler = ntrc_gameCmd0Handler;
    gNtrRomEmu.cmd1Handler = ntrc_gameCmd1Handler;
    gNtrRomEmu.mode = NTR_CARD_MODE_GAME;
}
