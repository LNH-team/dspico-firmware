#include "common.h"
#include "romData.h"
#include "ntrCardRom.h"

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
        #ifdef DETECT_CONSOLE_TYPE
            romEmu->previousCommand = NTR_CMD_ID_NORMAL_LOAD_TABLE;
        #endif
            break;
        }

        case NTR_CMD_ID_NORMAL_3DS_DETECT:
        {
            ntrc_noPayload(pio);
        #ifdef DETECT_CONSOLE_TYPE
            romEmu->isDSMode = false;
        #endif
            break;
        }

        case NTR_CMD_ID_NORMAL_READ_ID:
        {
        #ifdef DETECT_CONSOLE_TYPE
            if (romEmu->previousCommand == NTR_CMD_ID_NORMAL_LOAD_TABLE) // This command order is used on DSi
            {
                romEmu->isDSMode = false;
            }
            romEmu->previousCommand = NTR_CMD_ID_NORMAL_READ_ID;
        #endif
            ntrc_beginWrite(pio, 4);
            ntrc_writeWord(pio, romEmu->cardId);
            break;
        }

        case NTR_CMD_ID_NORMAL_READ_PAGE:
        {
        #ifdef DETECT_CONSOLE_TYPE
            if (romEmu->previousCommand == NTR_CMD_ID_NORMAL_LOAD_TABLE && romEmu->isDSMode)
            {
                //Console is DS, load normal rom in romData
                romEmu->romData = gDefaultRom;
                romEmu->romSize = (u32)gDefaultRomSize;
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
