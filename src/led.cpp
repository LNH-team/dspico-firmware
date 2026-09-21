#ifdef ENABLE_STATUS_LEDS

#include "common.h"
#include "led.h"
#include "hardware/gpio.h"
#include "hardware/structs/sio.h"
#include "pico/platform.h"

// How long the blue LED is held on after a write, counted in main loop passes.
// There is no usable wall clock here: power saving gates the timer clock, so
// everything below counts passes instead of milliseconds. Too low and a write
// looks like a read, too high and consecutive writes merge into one long blob.
#define LED_WRITE_HOLD_PASSES   20000u

// How long a read keeps the LED alive after the last one started. Only has to
// bridge the gap between two transfers of a game that is streaming, so it is far
// shorter than the write hold - that difference is what makes a save stand out.
#define LED_READ_HOLD_PASSES    2000u

// Reads are dimmed by lighting the LED on only one pass in 2^N. A running game
// pulls its rom off the card almost continuously, so at full brightness the LED
// would sit solid and stop carrying any information. Writes stay at full duty.
#define LED_READ_DUTY_SHIFT     3u

// Counters live outside the module so the notify helpers can stay inline.
volatile uint32_t gLedSdReadEvents = 0;
volatile uint32_t gLedSdWriteEvents = 0;

static u32 sSeenReadEvents = 0;
static u32 sSeenWriteEvents = 0;
static u32 sReadHold = 0;
static u32 sWriteHold = 0;
static u32 sPass = 0;

void ledInit(void)
{
    // Set the level before the direction so the pin never glitches a flash of
    // light at boot, and keep the 2 mA drive the other outputs here use.
    gpio_set_drive_strength(PIN_LED_R, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(PIN_LED_B, GPIO_DRIVE_STRENGTH_2MA);
    gpio_disable_pulls(PIN_LED_R);
    gpio_disable_pulls(PIN_LED_B);
    gpio_put(PIN_LED_R, false);
    gpio_put(PIN_LED_B, false);
    gpio_set_dir(PIN_LED_R, GPIO_OUT);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
}

// ledUpdate and ledPrepareForSleep run once per main loop pass, called from
// main, which is itself RAM resident (__time_critical_func). They are put in RAM
// too, and that is not a nicety: with them in flash the DSpico's USB device
// never enumerated on the host. The DS side of the usb examples polls the
// cartridge for USB events as fast as it can, so the main loop is woken and run
// at that rate, and every pass then fetched these two from flash through a
// veneer - the same flash the card handlers and the rom data compete for. This
// was bisected on hardware: the same build with the two calls removed
// enumerates, the build with the two in RAM enumerates, the build with them in
// flash does not. Why a few flash fetches per pass are enough to stop USB is not
// pinned down; what is known is that this code must not make them.
void __time_critical_func(ledUpdate)(void)
{
    uint32_t reads = gLedSdReadEvents;
    uint32_t writes = gLedSdWriteEvents;
    if (writes != sSeenWriteEvents)
    {
        sSeenWriteEvents = writes;
        sWriteHold = LED_WRITE_HOLD_PASSES;
    }
    if (reads != sSeenReadEvents)
    {
        sSeenReadEvents = reads;
        sReadHold = LED_READ_HOLD_PASSES;
    }

    sPass++;
    bool dimTick = (sPass & ((1u << LED_READ_DUTY_SHIFT) - 1u)) == 0u;
    gpio_put(PIN_LED_B, sWriteHold != 0 || (sReadHold != 0 && dimTick));

    if (sWriteHold != 0)
    {
        sWriteHold--;
    }
    if (sReadHold != 0)
    {
        sReadHold--;
    }
}

void __time_critical_func(ledPrepareForSleep)(void)
{
    gpio_put(PIN_LED_B, false);
}

// Replaces the SDK's default HardFault handler, which parks the core on a
// breakpoint. Every fault the firmware cannot come back from ends here: the
// terminal paths in the card handlers hit __breakpoint(), which with no
// debugger attached escalates to a HardFault, and so does anything else that
// goes wrong at that level. Lighting red from here rather than from those six
// paths keeps every byte of this feature out of the __scratch_y handlers (see
// led.h for why that margin matters), and covers faults the six paths never
// would have. Blue off first: a fault taken while it happened to be lit would
// otherwise strand it on next to the red one and muddle the signal. Direct SIO
// writes, because the gpio functions live in flash and may be what faulted.
extern "C" void isr_hardfault(void)
{
    sio_hw->gpio_clr = 1u << PIN_LED_B;
    sio_hw->gpio_oe_set = 1u << PIN_LED_R;
    sio_hw->gpio_set = 1u << PIN_LED_R;
    while (true)
    {
    }
}

#endif // ENABLE_STATUS_LEDS
