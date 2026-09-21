#pragma once
#include <stdint.h>

// Deliberately does NOT include common.h: that pulls in SdCard.h, which used to
// include this header back.

#ifdef ENABLE_STATUS_LEDS

// Bumped once every time an SD transfer actually starts, from SdCard's state
// machine (StateReadBegin / StateWriteBegin), which every transfer passes
// through: the interrupt driven ones and the blocking ones alike. An r4 mode rom
// read or a FatFs write begins AND finishes inside one main loop pass, so
// sampling the card state from that loop would never see it; counting at the
// source does.
//
// Where the count happens matters for a reason that is not obvious: the card
// command handlers are __scratch_y, and SCRATCH_Y is also where the core 0
// stack lives, growing down onto the top of that code with only a few hundred
// bytes to spare. Every byte this feature puts into those handlers - an inlined
// increment, a call and its long-branch veneer - comes straight out of that
// margin, and the top of the bank is exactly where the veneers into the USB
// code sit. So nothing here is called from a __scratch_y function, and the
// build must leave __scratch_y_end__ where upstream has it.
//
// An increment can in principle be lost if two contexts race here. The cost of
// that is one missed blink, which is not worth an interrupt disable.
extern volatile uint32_t gLedSdReadEvents;
extern volatile uint32_t gLedSdWriteEvents;

/// @brief Records that a read has started. One add, no branch, no pin access.
static inline void ledNotifySdRead(void) { gLedSdReadEvents++; }

/// @brief Records that a write has started. See ledNotifySdRead.
static inline void ledNotifySdWrite(void) { gLedSdWriteEvents++; }

/// @brief Claims the two LED pins and parks both dark.
void ledInit(void);

/// @brief Drives the blue LED from the counters above. Call once per main loop
///        pass. RAM resident, and it has to be - see led.cpp.
void ledUpdate(void);

/// @brief Clears the blue LED before the main loop sleeps.
///
/// This has to happen every pass. The loop only runs when an interrupt wakes it,
/// so a level left set here stays set for as long as the card bus is quiet, and
/// the holds cannot count down either. Leaving it lit through __wfi to make
/// writes more obvious was tried and stranded the LED solid on hardware: a game
/// that saved and then ran from RAM left it blue forever. Brightness has to come
/// from duty cycle alone.
void ledPrepareForSleep(void);

// The red LED has no function to call. It is lit by the HardFault handler that
// led.cpp installs: the terminal paths in the card handlers end in
// __breakpoint(), which with no debugger attached escalates to a HardFault, and
// so does anything else the firmware cannot come back from. That is the one
// place that sees them all, and it costs the __scratch_y handlers nothing.

#else

// Every call site stays as it is and compiles to nothing, so a build without the
// flag behaves exactly as it did before - see the note in CMakeLists.txt.
static inline void ledNotifySdRead(void) { }
static inline void ledNotifySdWrite(void) { }
static inline void ledInit(void) { }
static inline void ledUpdate(void) { }
static inline void ledPrepareForSleep(void) { }

#endif
