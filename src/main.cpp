#include "common.h"
#include <string.h>
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "pico/binary_info.h"
#include "ntrCard.pio.h"
#include "romData.h"
#include "pico/multicore.h"
#include "blowfish.h"
#include "scrambler.h"
#include "sd/fatfs/ff.h"
#include "ntrCardRom.h"
#include "ntrCardSpiUart.h"
#include "r4.h"
#include "sd/SdCard.h"
#include "scramblerRing.h"
#include "pico/bootrom.h"
#include "hardware/xosc.h"
#include "powerSaving.h"

static u32 sProgramOffset;
FATFS sFatFs;
SdCard gSdCard;
static bool sIsSdCardMounted;

#ifdef DETECT_CONSOLE_TYPE
static void setRomToDsiRom(void)
{
    gNtrRomEmu.romData = gDsiRom;
    gNtrRomEmu.romSize = ((u32)gDsiRomSize + 511) & ~511;
    gNtrRomEmu.cardId = CARD_ID_TWL;
	gNtrRomEmu.isDSMode = true;
}
#endif
static void setRomToMainRom(void)
{
    gNtrRomEmu.romData = gDefaultRom;
    gNtrRomEmu.romSize = (u32)gDefaultRomSize;
    gNtrRomEmu.romSize = (gNtrRomEmu.romSize + 511) & ~511;
    // We support DSi mode if the firmware is dual mode, or if a single rom has the DSi flag set
	gNtrRomEmu.cardId = CARD_ID_TWL;
#ifndef DETECT_CONSOLE_TYPE
    if ((gDefaultRom[0x12] & 2) == 0)
    {
        gNtrRomEmu.cardId = CARD_ID_NTR;
    }
#endif
#if defined(DETECT_CONSOLE_TYPE) || defined(ENABLE_NTRBOOT)
    gNtrRomEmu.isDSMode = true;
#endif
}

static void resetNtrCard(void)
{
    ntrc_resetUsb();
    pwr_disableAfterBootPowerSaving();
    ntrc_setNormalMode();
    gNtrRomEmu.securePhase1 = false;
    gNtrRomEmu.cmdScramble = false;
    gNtrRomEmu.dataScramble = false;
    gComputeScrambler = false;
    gNtrRomEmu.scrRingRPtr = gScramblerRing;
    gScramblerRingWPtr = gScramblerRing;
    gNtrRomEmu.wordIdx = 0;
    gNtrRomEmu.twlMode = false;
    gNtrRomEmu.readDataDestination = nullptr;
    gNtrRomEmu.readDataCompleteHandler = nullptr;
    gNtrRomEmu.readDataLimit = 0;
#ifdef ENABLE_R4_MODE
    ntrc_resetR4();
#endif
    dma_channel_abort(0);
    pio_sm_set_enabled(pio0, 0, false);
    pio_sm_set_pindirs_with_mask(pio0, 0, 0, PIN_INPUT_MASK);
    pio_sm_clear_fifos(pio0, 0);
    pio_sm_restart(pio0, 0);
    pio_sm_clkdiv_restart(pio0, 0);
    irq_clear(PIO0_IRQ_0);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio_sm_exec(pio0, 0, pio_encode_jmp(sProgramOffset));
    pio_sm_set_enabled(pio0, 0, true);
#if defined(ENABLE_NTRBOOT)
	clocks_hw->sleep_en0 &= ~CLOCKS_ENABLED0_CLK_SYS_CLOCKS_BITS;
#endif
#ifdef DETECT_CONSOLE_TYPE
    setRomToDsiRom();
#elif defined(ENABLE_NTRBOOT)
	setRomToMainRom();
#endif
#ifdef DSPICO_ENABLE_WRFUXXED
    ntrc_resetSpiUart();
#endif
}

#ifdef ENABLE_PREVENT_DSI_AUTOBOOT
static u64 sResetStart;
#endif

static void __time_critical_func(gpioIrq)(uint gpio, u32 events)
{
#ifdef ENABLE_PREVENT_DSI_AUTOBOOT
    u64 time = time_us_64();
#endif
    if (gpio == PIN_RST)
    {
        if (events & GPIO_IRQ_EDGE_FALL)
        {
            pio_sm_set_enabled(pio0, 0, false);
            pio_sm_set_pindirs_with_mask(pio0, 0, 0, PIN_INPUT_MASK);
        }
        if (events & GPIO_IRQ_EDGE_RISE)
        {
            resetNtrCard();
        #ifdef ENABLE_PREVENT_DSI_AUTOBOOT
            u32 resetTime = time - sResetStart;
            if (resetTime > 700000)
                pio_sm_set_enabled(pio0, 0, false);
            sResetStart = time;
        #endif
        }   
    }        
}

void __scratch_x("cpu1") core1_entry(void)
{
    irq_set_mask_enabled(~0u, false);
    scb_hw->scr |= M0PLUS_SCR_SLEEPDEEP_BITS;
    while (!gComputeScrambler)
    {
        gScramblerRingWPtr = gScramblerRing;
        __wfe();
    }
    while (1)
    {
        u32* wPtr = gScramblerRingWPtr;
        u32* next = SCR_RING_WRAP(wPtr + 1);
        if (next == gNtrRomEmu.scrRingRPtr)
        {
            __wfe();
            continue;
        }

        *wPtr = scr_getNext32(&gScramblerState);
        gScramblerRingWPtr = next;
    }
}

static void initSd(void)
{
    memset(&sFatFs, 0, sizeof(sFatFs));

    //try mounting 16 times
    bool ok = false;
    for (int i = 0; i < 16; i++)
    {
        FRESULT mountResult = f_mount(&sFatFs, "0:", 1);
        if (mountResult == FR_OK)
        {
            ok = true;
            sIsSdCardMounted = true;
            break;
        }
        else if (mountResult == FR_NO_FILESYSTEM)
        {
            break;
        }
    }
    if (!ok)
    {
        sIsSdCardMounted = false;
    }
}

static void tryRebootToBootsel(void)
{
    if (!sIsSdCardMounted)
    {
        xosc_init();
        reset_usb_boot(0, 0);
    }
}

int __time_critical_func(main)()
{
    bi_decl(bi_program_description("Ntr card emulator"));
    bi_decl(bi_pin_mask_with_name(0xFF000, "Ntr card D0-D7"));
    bi_decl(bi_1pin_with_name(PIN_IRQ, "Ntr card irq"));
    bi_decl(bi_1pin_with_name(PIN_CEB, "Ntr card ceb (rom enable)"));
    bi_decl(bi_1pin_with_name(PIN_WREB, "Ntr card wreb (clock)"));
    bi_decl(bi_1pin_with_name(PIN_RST, "Ntr card reset"));
    bi_decl(bi_1pin_with_name(PIN_CS2, "Ntr card cs2 (spi enable)"));

    // u64 bootTime = time_us_64();

    // set_sys_clock_khz(/*125000*/200000, true);
    // 200 MHz = 1200 MHz / 6 / 1
    set_sys_clock_pll(1200000000, 6, 1);

    dma_channel_claim(0);

    memset(&gNtrRomEmu, 0, sizeof(gNtrRomEmu));

    multicore_launch_core1(core1_entry);

    gpio_init_mask(PIN_INPUT_MASK);
    gpio_set_dir_in_masked(PIN_INPUT_MASK);
    gpio_init(PIN_IRQ);
    gpio_put(PIN_IRQ, 0);
    gpio_set_dir(PIN_IRQ, GPIO_OUT);
    gpio_disable_pulls(PIN_D0);
    gpio_disable_pulls(PIN_D1);
    gpio_disable_pulls(PIN_D2);
    gpio_disable_pulls(PIN_D3);
    gpio_disable_pulls(PIN_D4);
    gpio_disable_pulls(PIN_D5);
    gpio_disable_pulls(PIN_D6);
    gpio_disable_pulls(PIN_D7);
    gpio_set_slew_rate(PIN_D0, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(PIN_D1, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(PIN_D2, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(PIN_D3, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(PIN_D4, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(PIN_D5, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(PIN_D6, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(PIN_D7, GPIO_SLEW_RATE_FAST);
    gpio_pull_up(PIN_CEB);
    gpio_pull_up(PIN_WREB);
    gpio_pull_down(PIN_RST);
    gpio_pull_up(PIN_CS2);
    gpio_set_drive_strength(PIN_D0, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(PIN_D1, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(PIN_D2, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(PIN_D3, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(PIN_D4, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(PIN_D5, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(PIN_D6, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(PIN_D7, GPIO_DRIVE_STRENGTH_2MA);

#if !defined(DETECT_CONSOLE_TYPE) && !defined(ENABLE_NTRBOOT)
	setRomToMainRom();
#endif

    sProgramOffset = pio_add_program(pio0, &ntr_card_program);
#ifdef DSPICO_ENABLE_WRFUXXED
    u32 spiUartProgOffs = pio_add_program(pio0, &ntr_card_spi_program);
#endif
    pio_sm_config c = ntr_card_program_get_default_config(sProgramOffset);
    sm_config_set_out_pins(&c, PIN_D0, 8);
    sm_config_set_in_pins(&c, PIN_D0);
    sm_config_set_sideset_pins(&c, PIN_D5);
    sm_config_set_set_pins(&c, PIN_D0, 5);
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_clkdiv(&c, 1);
    pio_sm_set_pindirs_with_mask(pio0, 0, 0, PIN_INPUT_MASK);
    pio_sm_set_pins_with_mask(pio0, 0, 0, PIN_INPUT_MASK);
    pio0->input_sync_bypass = 0xFF000;
    pio_gpio_init(pio0, PIN_D0);
    pio_gpio_init(pio0, PIN_D1);
    pio_gpio_init(pio0, PIN_D2);
    pio_gpio_init(pio0, PIN_D3);
    pio_gpio_init(pio0, PIN_D4);
    pio_gpio_init(pio0, PIN_D5);
    pio_gpio_init(pio0, PIN_D6);
    pio_gpio_init(pio0, PIN_D7);

    pio_sm_init(pio0, 0, sProgramOffset, &c);
    pio_set_irq0_source_enabled(pio0, pis_sm0_rx_fifo_not_empty, true);
    irq_set_exclusive_handler(PIO0_IRQ_0, ntrc_pioIrq);
#ifdef DSPICO_ENABLE_WRFUXXED
    ntrc_initSpiUart(spiUartProgOffs);
#endif

    gpio_set_irq_callback(gpioIrq);
    gpio_set_irq_enabled(PIN_RST, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    irq_init_priorities();
#ifdef ENABLE_NTRBOOT
	// Setup the systick timer with the processor clock as clock source (running at 200mhz, we get a 5ns resolution)
	systick_hw->csr = 0x5;
	// Sets the reload value to the maximum
	systick_hw->rvr = 0x00ffffff;
#endif
    irq_set_priority(PIO0_IRQ_0, 0x40);
    irq_set_priority(IO_IRQ_BANK0, 0x40);
    irq_set_priority(USBCTRL_IRQ, 0x80);
    irq_set_priority(DMA_IRQ_1, 0x80);
    irq_set_priority(TIMER_IRQ_0, 0x80);

    // printf("Starting\n");
    // printf("sProgramOffset %d\n", sProgramOffset);
    // printf("Boot time %d\n", (u32)bootTime);
    resetNtrCard();
    sIsSdCardMounted = false;
    initSd();

    tryRebootToBootsel();

    pwr_initPowerSaving();

    while (1)
    {
        gSdCard.Update();
        gSdCard.Update();
    #ifdef ENABLE_R4_MODE
        ntrc_gameR4Update();
    #endif
        __wfi();
    }
}
