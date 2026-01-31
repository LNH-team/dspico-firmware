#include "common.h"
#include <stdlib.h>
#include "romData.h"
#include "ntrCardRom.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"

#define ROM_HEADER_TWL_AREA_START_OFFSET    0x92
#define TWL_AREA_START_STEP_SIZE            0x80000
#define NTR_P_TABLE_OFFSET                  0x1600
#define NTR_S_BOXES_OFFSET                  0x1C00
#define TWL_P_TABLE_OFFSET                  0x600
#define TWL_S_BOXES_OFFSET                  0xC00

static void __time_critical_func(normCmd0Handler)(struct ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    romEmu->cmd0 = word;
    switch (word >> 24)
    {
        case NTR_CMD_ID_NORMAL_LOAD_TABLE:
        {
            ntrc_noPayload(pio); //ignore load table cycles
        #if defined(DETECT_CONSOLE_TYPE) || defined(ENABLE_NTRBOOT_AUTO_DETECTION)
            romEmu->previousCommand = NTR_CMD_ID_NORMAL_LOAD_TABLE;
        #endif
        #ifdef ENABLE_NTRBOOT_AUTO_DETECTION
            // Enable the systick clock and reload the counter
            clocks_hw->sleep_en0 |= CLOCKS_ENABLED0_CLK_SYS_CLOCKS_BITS;
            systick_hw->cvr = 0;
        #endif
            break;
        }

        case NTR_CMD_ID_NORMAL_3DS_DETECT:
        {
            ntrc_noPayload(pio);
        #if defined(DETECT_CONSOLE_TYPE) || defined(ENABLE_NTRBOOT_AUTO_DETECTION)
            romEmu->isDSMode = false;
        #endif
        #ifdef ENABLE_NTRBOOT_AUTO_DETECTION
            // We're not in a ntrboot context, disable the systick clock
            clocks_hw->sleep_en0 &= ~CLOCKS_ENABLED0_CLK_SYS_CLOCKS_BITS;
        #endif
            break;
        }

        case NTR_CMD_ID_NORMAL_READ_ID:
        {
        #if defined(DETECT_CONSOLE_TYPE) || defined(ENABLE_NTRBOOT_AUTO_DETECTION)
            if (romEmu->previousCommand == NTR_CMD_ID_NORMAL_LOAD_TABLE) // This command order is used on DSi/3DS
            {
                romEmu->isDSMode = false;
        #ifdef ENABLE_NTRBOOT_AUTO_DETECTION
                // We're not in a ntrboot context, disable the systick clock
                clocks_hw->sleep_en0 &= ~CLOCKS_ENABLED0_CLK_SYS_CLOCKS_BITS;
        #endif
            }
            romEmu->previousCommand = NTR_CMD_ID_NORMAL_READ_ID;
        #endif
            ntrc_beginWrite(pio, 4);
            ntrc_writeWord(pio, romEmu->cardId);
            break;
        }

        case NTR_CMD_ID_NORMAL_READ_PAGE:
        {
        #if defined(DETECT_CONSOLE_TYPE) || defined(ENABLE_NTRBOOT_AUTO_DETECTION)
            if (romEmu->previousCommand == NTR_CMD_ID_NORMAL_LOAD_TABLE && romEmu->isDSMode)
            {
            #ifdef ENABLE_NTRBOOT_AUTO_DETECTION
                u32 curtick = systick_hw->cvr;
                // We no longer need the systick to be active, disable its clock
                clocks_hw->sleep_en0 &= ~CLOCKS_ENABLED0_CLK_SYS_CLOCKS_BITS;
                // the 3ds takes around 2076245 nanoseconds to send the command after sending 9f (0x65611 ticks)
                // on top of that we add a bit more of leeway and we wait for 2109440 nanoseconds (0x67000 ticks)
                // so if the elapsed time is more than that amount, we're no longer a being read by a 3DS and we
                // serve again the DS rom
                if (curtick < 0xF98FFF)
                {
                    //Console is DS, load normal rom in romData
                    romEmu->romData = gDefaultRom;
                    romEmu->romSize = (u32)gDefaultRomSize;
                }
            #ifdef ENABLE_NTRBOOT_CONSOLE_TYPE_DETECTION
                // the DSi takes around 2067540 nanoseconds to send the command after sending 9f (0x646FF ticks)
                // so if the elapsed time is more than that amount, we're no longer a being read by a DSi
                else if (curtick < 0xF9A9EE)
                {
                    //Console is 3DS, trying to load a ntrboot image, load 3ds ntrboot rom
                    romEmu->romData = gNtrbootRom;
                    romEmu->romSize = (u32)gNtrbootRomSize;
                }
                // the ds takes around 2056020 nanoseconds to send the command after sending 9f (0x64644 ticks)
                // so if the elapsed time is more than that amount, we're no longer a being read by a ds
                else if (curtick < 0xF9B9BB)
                {
                    //Console is DSi, trying to load a ntrboot image, load dsi ntrboot rom
                    romEmu->romData = gNtrbootDsiRom;
                    romEmu->romSize = (u32)gNtrbootDsiRomSize;
                }
            #else
                // the ds takes around 2056020 nanoseconds to send the command after sending 9f (0x64644 ticks)
                // so if the elapsed time is more than that amount, we're no longer a being read by a ds
                else if (curtick < 0xF9B9BB)
                {
                    //Console is DSi or 3DS, trying to load a ntrboot image, load ntrboot rom
                    romEmu->romData = gNtrbootRom;
                    romEmu->romSize = (u32)gNtrbootRomSize;
                }
            #endif
                else
            #endif
                {
                    //Console is DS, load normal rom in romData
                    romEmu->romData = gDefaultRom;
                    romEmu->romSize = (u32)gDefaultRomSize;
                }
                romEmu->romSize = (romEmu->romSize + 511) & ~511;
                romEmu->cardId = CARD_ID_NTR;
            }
            romEmu->previousCommand = NTR_CMD_ID_NORMAL_READ_PAGE;
        #endif
            ntrc_beginWrite(pio, 512);
            u32 address = (word << 8) & 0xFFF;
            ntrc_dmaToBus(&romEmu->romData[address], 512);
            break;
        }

        case NTR_CMD_ID_NORMAL_CHANGE_MODE_NTR:
        {
            ntrc_noPayload(pio); //no data
            break;
        }

        case NTR_CMD_ID_NORMAL_CHANGE_MODE_TWL:
        {
            ntrc_noPayload(pio); //no data
            break;
        }

        default:
        {
            ntrc_noPayload(pio);
            break;
        }
    }
    romEmu->wordIdx = 1;
}

static void __time_critical_func(normCmd1Handler)(struct ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    romEmu->cmd1 = word;
    if ((romEmu->cmd0 >> 24) == NTR_CMD_ID_NORMAL_CHANGE_MODE_NTR)
    {
        ntrc_setSecureMode();
        romEmu->cmdScramble = false;
        romEmu->dataScramble = false;
        //ntr blowfish
        bf_init(&romEmu->blowfish, (const u32*)&romEmu->romData[NTR_P_TABLE_OFFSET],
            (const bf_sboxes_t*)&romEmu->romData[NTR_S_BOXES_OFFSET]);
    }
    else if ((romEmu->cmd0 >> 24) == NTR_CMD_ID_NORMAL_CHANGE_MODE_TWL)
    {
        ntrc_setSecureMode();
        romEmu->cmdScramble = false;
        romEmu->dataScramble = false;
        romEmu->twlMode = true;
        //twl blowfish
        u32 twlAreaStart = *(const u16*)&romEmu->romData[ROM_HEADER_TWL_AREA_START_OFFSET] * TWL_AREA_START_STEP_SIZE;
        bf_init(&romEmu->blowfish, (const u32*)&romEmu->romData[twlAreaStart + TWL_P_TABLE_OFFSET],
            (const bf_sboxes_t*)&romEmu->romData[twlAreaStart + TWL_S_BOXES_OFFSET]);
    }
    romEmu->wordIdx = 0; //no normal commands have extra payload to read
}

void __time_critical_func(ntrc_setNormalMode)(void)
{
    gNtrRomEmu.cmd0Handler = normCmd0Handler;
    gNtrRomEmu.cmd1Handler = normCmd1Handler;
    gNtrRomEmu.mode = NTR_CARD_MODE_NORMAL;
}
