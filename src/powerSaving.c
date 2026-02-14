#include "common.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/rosc.h"
#include "hardware/structs/syscfg.h"
#include "hardware/structs/systick.h"

static void stopUnusedClocks(void)
{
    clock_stop(clk_gpout0);
    clock_stop(clk_gpout1);
    clock_stop(clk_gpout2);
    clock_stop(clk_gpout3);
    clock_stop(clk_usb);
    clock_stop(clk_adc);
    clock_stop(clk_rtc);
    clock_stop(clk_peri);
    pll_deinit(pll_usb);
    uint32_t tmp = rosc_hw->ctrl;
    tmp &= ~ROSC_CTRL_ENABLE_BITS;
    tmp |= ROSC_CTRL_ENABLE_VALUE_DISABLE << ROSC_CTRL_ENABLE_LSB;
    hw_clear_bits(&rosc_hw->status, ROSC_STATUS_BADWRITE_BITS);
    rosc_hw->ctrl = tmp;
    clocks_hw->wake_en0 &= ~(
        CLOCKS_WAKE_EN0_CLK_ADC_ADC_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_ADC_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_I2C0_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_I2C1_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_PWM_BITS |
        CLOCKS_WAKE_EN0_CLK_RTC_RTC_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_RTC_BITS |
        CLOCKS_WAKE_EN0_CLK_PERI_SPI0_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_SPI0_BITS |
        CLOCKS_WAKE_EN0_CLK_PERI_SPI1_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_SPI1_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_ROSC_BITS |
        CLOCKS_WAKE_EN0_CLK_SYS_PLL_USB_BITS);
    clocks_hw->wake_en1 &= ~(
        CLOCKS_WAKE_EN1_CLK_SYS_UART0_BITS |
        CLOCKS_WAKE_EN1_CLK_PERI_UART0_BITS |
        CLOCKS_WAKE_EN1_CLK_SYS_UART1_BITS |
        CLOCKS_WAKE_EN1_CLK_PERI_UART1_BITS |
        CLOCKS_WAKE_EN1_CLK_SYS_WATCHDOG_BITS |
        CLOCKS_WAKE_EN1_CLK_SYS_TIMER_BITS);
}

static void initDeepSleep(void)
{
    // set clocks enabled during deep sleep
    clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_CLK_SYS_PIO0_BITS;
    clocks_hw->sleep_en1 = 0x0;

    // enable deep sleep
    scb_hw->scr |= M0PLUS_SCR_SLEEPDEEP_BITS;
}

void pwr_initPowerSaving(void)
{
    stopUnusedClocks();
    initDeepSleep();
}

void pwr_enableAfterBootPowerSaving(void)
{
    hw_set_bits(&syscfg_hw->mempowerdown, SYSCFG_MEMPOWERDOWN_USB_BITS);
    hw_clear_bits(&clocks_hw->wake_en1,
        CLOCKS_WAKE_EN1_CLK_USB_USBCTRL_BITS |
        CLOCKS_WAKE_EN1_CLK_SYS_USBCTRL_BITS);
}

void pwr_disableAfterBootPowerSaving(void)
{
    hw_set_bits(&clocks_hw->wake_en1,
        CLOCKS_WAKE_EN1_CLK_USB_USBCTRL_BITS |
        CLOCKS_WAKE_EN1_CLK_SYS_USBCTRL_BITS);
    hw_clear_bits(&syscfg_hw->mempowerdown, SYSCFG_MEMPOWERDOWN_USB_BITS);
}

void pwr_disableUsbPowerSaving(void)
{
    hw_set_bits(&clocks_hw->wake_en0, CLOCKS_WAKE_EN0_CLK_SYS_PLL_USB_BITS);
    hw_set_bits(&clocks_hw->sleep_en0, CLOCKS_SLEEP_EN0_CLK_SYS_PLL_USB_BITS);
    hw_set_bits(&clocks_hw->wake_en1,
        CLOCKS_WAKE_EN1_CLK_USB_USBCTRL_BITS |
        CLOCKS_WAKE_EN1_CLK_SYS_USBCTRL_BITS);
    hw_set_bits(&clocks_hw->sleep_en1,
        CLOCKS_SLEEP_EN1_CLK_USB_USBCTRL_BITS |
        CLOCKS_SLEEP_EN1_CLK_SYS_USBCTRL_BITS);
    hw_clear_bits(&syscfg_hw->mempowerdown, SYSCFG_MEMPOWERDOWN_USB_BITS);
    pll_init(pll_usb, 1, 1200000000, 5, 5);
    clock_configure(
        clk_usb,
        0,
        CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
        48 * MHZ,
        48 * MHZ);
}

void pwr_enableUsbPowerSaving(void)
{
    clock_stop(clk_usb);
    pll_deinit(pll_usb);
    hw_clear_bits(&clocks_hw->wake_en0, CLOCKS_WAKE_EN0_CLK_SYS_PLL_USB_BITS);
    hw_clear_bits(&clocks_hw->sleep_en0, CLOCKS_SLEEP_EN0_CLK_SYS_PLL_USB_BITS);
    hw_clear_bits(&clocks_hw->wake_en1,
        CLOCKS_WAKE_EN1_CLK_USB_USBCTRL_BITS |
        CLOCKS_WAKE_EN1_CLK_SYS_USBCTRL_BITS);
    hw_clear_bits(&clocks_hw->sleep_en1,
        CLOCKS_SLEEP_EN1_CLK_USB_USBCTRL_BITS |
        CLOCKS_SLEEP_EN1_CLK_SYS_USBCTRL_BITS);
    hw_set_bits(&syscfg_hw->mempowerdown, SYSCFG_MEMPOWERDOWN_USB_BITS);
}

void pwr_disableSysTickClock(void)
{
    hw_clear_bits(&clocks_hw->sleep_en0, CLOCKS_SLEEP_EN0_CLK_SYS_CLOCKS_BITS);
}

void pwr_enableSysTickClock(void)
{
    hw_set_bits(&clocks_hw->sleep_en0, CLOCKS_SLEEP_EN0_CLK_SYS_CLOCKS_BITS);
}
