#pragma once

#include "pico/stdlib.h"
#include "pico/time.h"

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef volatile uint8_t vu8;
typedef volatile int8_t vs8;
typedef volatile uint16_t vu16;
typedef volatile int16_t vs16;
typedef volatile uint32_t vu32;
typedef volatile int32_t vs32;
typedef volatile uint64_t vu64;
typedef volatile int64_t vs64;

#define SD_USE_SDIO

#define SDIO_CLK 3
#define SDIO_CMD 4
#define SDIO_D0  5
#define SDIO_D1  6
#define SDIO_D2  7
#define SDIO_D3  8

#define PIN_RST     9
#define PIN_CEB     10
#define PIN_WREB    11 //clk
#define PIN_D0      12
#define PIN_D1      13
#define PIN_D2      14
#define PIN_D3      15
#define PIN_D4      16
#define PIN_D5      17
#define PIN_D6      18
#define PIN_D7      19
#define PIN_IRQ     20
#define PIN_CS2     21

#define PIN_USB_VBUS    24

#define PIN_DEV_TX0     0
#define PIN_DEV_RX0     1

#define PIN_INPUT_MASK  0x2FFE00

#define CARD_ID_NTR     0x800000C2
#define CARD_ID_TWL     0xC00000C2

typedef void (*sd_callback_t)(uint32_t bytes_complete);

static inline uint millis(void)
{
    return us_to_ms(time_us_64());
}

#ifdef __cplusplus
#include "sd/SdCard.h"
extern SdCard gSdCard;
#endif
