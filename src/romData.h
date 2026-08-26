#pragma once

extern uint8_t gDefaultRom[];
extern uint8_t gDefaultRomSize[];

#ifdef DETECT_CONSOLE_TYPE

extern uint8_t gDsiRom[];
extern uint8_t gDsiRomSize[];

#endif

#ifdef ENABLE_NTRBOOT_AUTO_DETECTION

extern uint8_t gNtrbootRom[];
extern uint8_t gNtrbootRomSize[];

#ifdef ENABLE_NTRBOOT_CONSOLE_TYPE_DETECTION

extern uint8_t gNtrbootDsiRom[];
extern uint8_t gNtrbootDsiRomSize[];

#endif

#endif