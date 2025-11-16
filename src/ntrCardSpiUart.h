#pragma once
#include "common.h"

#ifdef DSPICO_ENABLE_WRFUXXED

#define NTRC_SPI_UART_CMD_ECHO_1                0x60
#define NTRC_SPI_UART_CMD_NOP                   0x80
#define NTRC_SPI_UART_CMD_RESET                 0x90
#define NTRC_SPI_UART_CMD_ECHO_2                0xA0
#define NTRC_SPI_UART_CMD_GET_WRITABLE_LENGTH   0xB0
#define NTRC_SPI_UART_CMD_GET_READABLE_LENGTH   0xC0
#define NTRC_SPI_UART_CMD_WRITE                 0xD0
#define NTRC_SPI_UART_CMD_READ                  0xE0
#define NTRC_SPI_UART_CMD_INIT                  0xF0

/// @brief Context struct for spi uart adapter emulation.
typedef struct
{
    u8 status;
    u8 retDataBuf[17];
    u8 retDataPtr;
    u8 retDataCnt;
    u8 writeDataCnt;
    const u8* readBufPtr;
    int readBufCount;
    u32 progOffs;
} ntrc_spi_uart_t;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Initializes spi uart adapter emulation.
/// @param programOffs The pio program offset to use.
void ntrc_initSpiUart(u32 programOffs);

/// @brief Resets the spi uart adapter.
void ntrc_resetSpiUart(void);

#ifdef __cplusplus
}
#endif

#endif
