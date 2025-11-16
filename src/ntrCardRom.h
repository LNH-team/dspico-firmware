#pragma once
#include "common.h"
#include "hardware/pio.h"
#include "blowfish.h"

typedef enum
{
    NTR_CARD_MODE_NORMAL,
    NTR_CARD_MODE_SECURE,
    NTR_CARD_MODE_GAME,
    NTR_CARD_MODE_GAME_NO_SCRAMBLE
} NtrCardMode;

// normal mode commands
#define NTR_CMD_ID_NORMAL_READ_PAGE         0x00 // read unprotected header page
#define NTR_CMD_ID_NORMAL_CHANGE_MODE_NTR   0x3C // change to secure mode (ntr)
#define NTR_CMD_ID_NORMAL_CHANGE_MODE_TWL   0x3D // change to secure mode (twl)
#define NTR_CMD_ID_NORMAL_READ_ID           0x90 // read card id
#define NTR_CMD_ID_NORMAL_LOAD_TABLE        0x9F // load blowfish table
#define NTR_CMD_ID_NORMAL_3DS_DETECT        0x71 // 3ds detection

// secure mode commands
#define NTR_CMD_ID_SECURE_READ_ID           0x1 // read card id
#define NTR_CMD_ID_SECURE_READ_SEGMENT      0x2 // read secure segment
#define NTR_CMD_ID_SECURE_SCRAMBLER_ON      0x4 // enable scrambler
#define NTR_CMD_ID_SECURE_SCRAMBLER_OFF     0x6 // disable scrambler
#define NTR_CMD_ID_SECURE_CHANGE_MODE       0xA // change to game mode

// game mode commands
#define NTR_CMD_ID_GAME_READ_PAGE           0xB7 // read rom page
#define NTR_CMD_ID_GAME_READ_ID             0xB8 // read card id

// custom game mode commands
#define NTR_CMD_ID_GAME_REQ_SD_READ         0xE3
#define NTR_CMD_ID_GAME_GET_SD_STAT         0xE4
#define NTR_CMD_ID_GAME_GET_SD_DATA         0xE5
#define NTR_CMD_ID_GAME_WRITE_SD_DATA       0xF6E10D98
#define NTR_CMD_ID_GAME_DISABLE_SCRAMBLING  0xFC

#define WRITE_SD_DATA_IS_FIRST_FLAG         2
#define WRITE_SD_DATA_IS_LAST_FLAG          1
#define WRITE_SD_DATA_FLAGS_MASK            (WRITE_SD_DATA_IS_FIRST_FLAG | WRITE_SD_DATA_IS_LAST_FLAG)

// custom game mode commands for USB
#define NTR_CMD_ID_GAME_USB_COMMAND         0xE8
#define NTR_CMD_ID_GAME_USB_WRITE_DATA      0xE9
#define NTR_CMD_ID_GAME_USB_READ_DATA       0xEA
#define NTR_CMD_ID_GAME_USB_GET_EVENT       0xEB

struct ntr_rom_emu_t;

typedef void (*ntrc_cmd_handler_t)(struct ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);
typedef void (*ntrc_game_cmd_handler_t)(struct ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);
typedef void (*ntrc_read_data_complete_handler_t)(struct ntr_rom_emu_t* romEmu);

typedef struct ntr_rom_emu_t
{
    ntrc_cmd_handler_t cmd0Handler;
    ntrc_cmd_handler_t cmd1Handler;

    int wordIdx;
    u32* volatile scrRingRPtr;
    u32 cmd0;
    u32 cmd1;
    u32* readDataDestination;
    u32 readDataLimit;
    ntrc_read_data_complete_handler_t readDataCompleteHandler;

    // These are not used by assembly code
    NtrCardMode mode;
    bool securePhase1;
    int secureBlock;
    bool cmdScramble;
    bool dataScramble;
    u32 cardId;
    bool twlMode;
#ifdef ENABLE_R4_MODE
    bool r4Mode;
#endif
    u32 romSize;
    bf_context_t blowfish;
#ifdef DETECT_CONSOLE_TYPE
    u16 previousCommand;
    bool isDSMode;
#endif
    u8* romData;
} ntr_rom_emu_t;

// These asserts make sure the assembly code doesn't break
// Do not change these asserts without updating the assembly code
static_assert(offsetof(ntr_rom_emu_t, cmd0Handler) == 0, "cmd0Handler field offset not equal to 0");
static_assert(offsetof(ntr_rom_emu_t, cmd1Handler) == 4, "cmd1Handler field offset not equal to 4");
static_assert(offsetof(ntr_rom_emu_t, wordIdx) == 8, "wordIdx field offset not equal to 8");
static_assert(offsetof(ntr_rom_emu_t, scrRingRPtr) == 0xC, "scrRingRPtr field offset not equal to 0xC");
static_assert(offsetof(ntr_rom_emu_t, cmd0) == 0x10, "cmd0 field offset not equal to 0x10");
static_assert(offsetof(ntr_rom_emu_t, cmd1) == 0x14, "cmd1 field offset not equal to 0x14");
static_assert(offsetof(ntr_rom_emu_t, readDataDestination) == 0x18, "readDataDestination field offset not equal to 0x18");
static_assert(offsetof(ntr_rom_emu_t, readDataLimit) == 0x1C, "readDataLimit field offset not equal to 0x1C");
static_assert(offsetof(ntr_rom_emu_t, readDataCompleteHandler) == 0x20, "readDataCompleteHandler field offset not equal to 0x20");

extern ntr_rom_emu_t gNtrRomEmu;

#ifdef __cplusplus
extern "C" {
#endif

void ntrc_pioIrq(void);

/// @brief Switches the rom emulator to normal mode.
void ntrc_setNormalMode(void);

/// @brief Switches the rom emulator to secure mode.
void ntrc_setSecureMode(void);

/// @brief Switches the rom emulator to game mode.
void ntrc_setGameMode(void);

/// @brief Switches the rom emulator to unscrambled game mode.
void ntrc_setGameNoScrambleMode(void);

void ntrc_gameCmd1Unknown(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);
void ntrc_gameCmd0Dummy(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);
void ntrc_gameCmd1Dummy(ntr_rom_emu_t* romEmu, u32 word, pio_hw_t* pio);

/// @brief Sends the specified \p data with the given \p length to the cartridge bus using DMA.
/// @param data The data to send to the cartridge bus.
/// @param length The length of the data to send. Must be a multiple of 4.
void ntrc_dmaToBus(const void* data, u32 length);

#ifdef ENABLE_R4_MODE
void ntrc_gameR4Update(void);
void ntrc_resetR4(void);
#endif

void ntrc_resetUsb(void);

/// @brief Signals to the cartridge bus \p pio that the current command has no payload.
/// @param pio The pio instance to use.
static inline void ntrc_noPayload(pio_hw_t* pio)
{
    pio->txf[0] = 0;
}

/// @brief Signals to the cartridge bus \p pio that the current command has a
///        DSpico -> DS payload of the specified number of \p bytes.
/// @param pio The pio instance to use.
/// @param bytes The number of payload bytes that will be send to the DS.
static inline void ntrc_beginWrite(pio_hw_t* pio, int bytes)
{
    pio->txf[0] = bytes - 1;
}

/// @brief Signals to the cartridge bus \p pio that the current command has a
///        DS -> DSpico payload of the specified number of \p bytes.
/// @param pio The pio instance to use.
/// @param bytes The number of payload bytes that will be received from the DS.
static inline void ntrc_beginRead(pio_hw_t* pio, int bytes)
{
    pio->txf[0] = 0x80000000 | (bytes - 1);
}

/// @brief Queues a \p word to the cartridge bus \p pio that will be send to the DS.
/// @note This function assumes there is sufficient space in the pio fifo.
/// @param pio The pio instance to use.
/// @param word The word to send to the DS.
static inline void ntrc_writeWord(pio_hw_t* pio, u32 word)
{
    pio->txf[0] = word;
}

/// @brief Queues a \p word to the cartridge bus \p pio that will be send to the DS.
///        If there is not enough space in the pio fifo, this function will block until space becomes available.
/// @param pio The pio instance to use.
/// @param word The word to send to the DS.
static inline void ntrc_writeWordBlocking(pio_hw_t* pio, u32 word)
{
    pio_sm_put_blocking(pio, 0, word);
}

#ifdef __cplusplus
}
#endif