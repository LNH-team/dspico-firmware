#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Initializes power saving.
void pwr_initPowerSaving(void);

/// @brief Enables additional power saving for when unscrambled game mode has been reached.
void pwr_enableAfterBootPowerSaving(void);

/// @brief Disables additional power saving such that scrambling can be used again.
void pwr_disableAfterBootPowerSaving(void);

/// @brief Disables USB power saving, such that USB can be used.
void pwr_disableUsbPowerSaving(void);

/// @brief Enables USB power saving. USB can no longer be used.
void pwr_enableUsbPowerSaving(void);

/// @brief Disables the SysTick in power saving mode. The SysTick register will no longer be updated.
void pwr_disableSysTickClock();

/// @brief Enables the SysTick in power saving mode. The SysTick register will be updated.
void pwr_enableSysTickClock();

#ifdef __cplusplus
}
#endif
