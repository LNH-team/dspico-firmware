#include "common.h"
#include "xor.h"
#include "romData.h"
#include "scramblerRing.h"
#include "ntrCardRom.h"

#define KEY2_DEFAULT_SEED0  0b101100011000101011011011110000011101000ULL
#define KEY2_DEFAULT_SEED1  0b101110010000111100110111001101100000101ULL

static const u8 sKey2SeedTable[] = { 0xE8, 0x4D, 0x5A, 0xB1, 0x17, 0x8F, 0x99, 0xD5 };

static void __time_critical_func(secureCmd0Handler)(struct ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    if (!romEmu->securePhase1)
    {
        romEmu->cmd0 = word;
        ntrc_noPayload(pio); // first phase is dummy
    }
    else
    {
        switch (romEmu->cmd0 >> 28)
        {
            case NTR_CMD_ID_SECURE_SCRAMBLER_ON:
            {
                ntrc_noPayload(pio);
                break;
            }

            case NTR_CMD_ID_SECURE_CHANGE_MODE:
            {
                ntrc_noPayload(pio);
                break;
            }

            case NTR_CMD_ID_SECURE_READ_ID:
            {
                ntrc_beginWrite(pio, 4);
                u32 cardId = romEmu->cardId;
                if (romEmu->dataScramble)
                {
                    cardId ^= *romEmu->scrRingRPtr;
                }
                ntrc_writeWord(pio, cardId);
                if (romEmu->dataScramble)
                {
                    scr_advanceRing(romEmu, 1);
                }
                break;
            }

            case NTR_CMD_ID_SECURE_READ_SEGMENT:
            {
                break;
            }

            default:
            {
                ntrc_noPayload(pio);
                break;
            }
        }
    }
    romEmu->wordIdx = 1;
}

static void __time_critical_func(secureCmd1Handler)(struct ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio)
{
    if (!romEmu->securePhase1)
    {
        u64 dec = bf_decrypt(&romEmu->blowfish, ((u64)romEmu->cmd0 << 32) | word);
        romEmu->cmd0 = dec >> 32;
        romEmu->cmd1 = (u32)dec;

        if ((romEmu->cmd0 >> 28) == NTR_CMD_ID_SECURE_READ_SEGMENT)
        {
            romEmu->secureBlock = 0;
        }

        romEmu->securePhase1 = true;
    }
    else
    {
        if ((romEmu->cmd0 >> 28) == NTR_CMD_ID_SECURE_READ_SEGMENT)
        {
            ntrc_beginWrite(pio, 512);

            u32 address = (romEmu->cmd0 & 0x0FFFF000) + romEmu->secureBlock * 512;
            if (romEmu->twlMode)
            {
                address += *(const u16*)&romEmu->romData[0x92] * 0x80000 - 0x1000;
            }
            u32* xorPtr = romEmu->scrRingRPtr;
            u32* inPtr = (u32*)&romEmu->romData[address];
            u32* outPtr = (u32*)&gScrambleBuffers[gFreeScrambleBuf][0];

            applyXor(inPtr, xorPtr, outPtr, 512);

            ntrc_dmaToBus(&gScrambleBuffers[gFreeScrambleBuf][0], 512);

            if (++romEmu->secureBlock == 8)
            {
                romEmu->secureBlock = 0;
                romEmu->securePhase1 = false;
            }

            romEmu->scrRingRPtr = SCR_RING_WRAP(xorPtr + 128);
            __sev();
            gFreeScrambleBuf = 1 - gFreeScrambleBuf;
        }
        else
        {
            romEmu->secureBlock = 0;
            romEmu->securePhase1 = false;

            if ((romEmu->cmd0 >> 28) == NTR_CMD_ID_SECURE_SCRAMBLER_ON)
            {
                u64 cmd = (((u64)romEmu->cmd0) << 32) | romEmu->cmd1;
                u32 mmmnnn = (u32)(cmd >> 20) & 0xFFFFFF;
                u8 key2SeedIdx = romEmu->romData[0x13] & 7;
                u64 seed0 = ((u64)mmmnnn << 15) | (0x60 << 8) | sKey2SeedTable[key2SeedIdx];
                scr_init(&gScramblerState, seed0);
                gNtrRomEmu.dataScramble = true;
                gNtrRomEmu.scrRingRPtr = gScramblerRing;
                gScramblerRingWPtr = gScramblerRing;
                gComputeScrambler = true;
                __sev();
            }
            else if ((romEmu->cmd0 >> 28) == NTR_CMD_ID_SECURE_CHANGE_MODE)
            {
                ntrc_setGameMode();
                if (romEmu->dataScramble)
                {
                    romEmu->cmdScramble = true;
                }
            }
        }
    }
    romEmu->wordIdx = 0; //no secure commands have extra payload to read
}

void __time_critical_func(ntrc_setSecureMode)(void)
{
    gNtrRomEmu.cmd0Handler = secureCmd0Handler;
    gNtrRomEmu.cmd1Handler = secureCmd1Handler;
    gNtrRomEmu.mode = NTR_CARD_MODE_SECURE;
    gNtrRomEmu.securePhase1 = false;
}
