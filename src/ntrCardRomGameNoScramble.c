#include "common.h"
#include <stdio.h>
#include "ntrCardRom.h"
#include "powerSaving.h"
#include "ntrCardRomGameNoScramble.h"

void ntrc_gameNoScrambleCmd1Unknown(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_noPayload(pio);
    ntrc_finishGameNoScrambleCmd1(romEmu);

    //do not receive further commands until card reset
    irq_set_enabled(PIO0_IRQ_0, false);

    puts("Unknown game command");
    printf("%08X%08X\n", romEmu->cmd0, romEmu->cmd1);
}

void __time_critical_func(ntrc_gameNoScrambleCmd0Dummy)(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

void __time_critical_func(ntrc_gameNoScrambleCmd1Dummy0)(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameNoScrambleCmd1(romEmu);
}

void __time_critical_func(ntrc_gameNoScrambleCmd1Dummy4)(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_finishGameNoScrambleCmd1(romEmu);
}

void __scratch_y("cpu0") ntrc_gameNoScrambleReadIdCmd0(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    ntrc_beginWrite(pio, 4);
    ntrc_writeWord(pio, romEmu->cardId);
    ntrc_finishGameNoScrambleCmd0(romEmu);
}

extern void ntrc_gameNoScrambleCmd0Handler(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);
extern void ntrc_gameNoScrambleCmd1Handler(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);

void __time_critical_func(ntrc_setGameNoScrambleMode)(void)
{
    gNtrRomEmu.cmd0Handler = ntrc_gameNoScrambleCmd0Handler;
    gNtrRomEmu.cmd1Handler = ntrc_gameNoScrambleCmd1Handler;
    gNtrRomEmu.mode = NTR_CARD_MODE_GAME_NO_SCRAMBLE;
    pwr_enableAfterBootPowerSaving();
}
