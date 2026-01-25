# DSpico Firmware
This is the repository for the DSpico firmware. The firmware emulates a DS cartridge, with extended features for SD access and USB. PIO is used for an SDIO interface for the SD card and for interfacing the DS cartridge bus.

For an overview of the supported card commands, see [commands.md](docs/commands.md).

## Features
- Emulates a retail DS(i) cartridge
- Interfaces with an SD card using SDIO and exposes card commands to access it from the DS side
- Exposes card commands to the DS side to allow interfacing with the USB port of the RP2040
- Can emulate an R4 to support software such as the Wood R4 kernel
- Supports having a separate rom for DS and DSi/3DS systems
- Supports emulating the IS-SPI-USB-ADAPTER for the WRFUxxed exploit
- Easy updating; starting the firmware with an ejected SD card reboots to BOOTSEL
- Optimized for minimal power use when idle

## Pinout
| **Peripheral**  | **Pin name - Peripheral** | **Pin name - RP2040** |
|-------------|-----------------------|-------------------|
| **DS Slot** | D0                    | GPIO12            |
|             | D1                    | GPIO13            |
|             | D2                    | GPIO14            |
|             | D3                    | GPIO15            |
|             | D4                    | GPIO16            |
|             | D5                    | GPIO17            |
|             | D6                    | GPIO18            |
|             | D7                    | GPIO19            |
|             | CLK_DS                | GPIO11            |
|             | ROM_CS                | GPIO10            |
|             | SPI_CS                | GPIO21            |
|             | IRQ                   | GPIO20            |
|             | RST_DS                | GPIO09            |
| **SDIO**    | CLK_SD                | GPIO03            |
|             | DAT0                  | GPIO05            |
|             | DAT1                  | GPIO06            |
|             | DAT2                  | GPIO07            |
|             | DAT3                  | GPIO08            |
|             | CMD                   | GPIO04            |

## Setup & configuration
We recommend using WSL (Windows Subsystem for Linux), or a Unix-based machine to compile this repository.
The steps provided will assume a Linux environment. Alternatively, you can run the `setup_environment.sh` bash script.

 1. Run `sudo  apt  update && sudo  apt  install  cmake  gcc-arm-none-eabi  build-essential  git`
 2. Clone this repository
 3. Run the following commands:
    ```bash
    git submodule update --init
    cd pico-sdk
    git submodule update --init
    cd ..
    ```
    Note that you shouldn't use `--recursive` because it draws in a lot of unnecessary submodules inside the pico-sdk.

### CMakeList
The `CMakeList.txt` file contains a couple of options that you can configure.

   * `ENABLE_R4_MODE` - Enables R4 emulation. This allows you to use R4 software, such as the Wood R4 kernel. As R4 emulation can be used together with regular DSpico software, it can usually be kept enabled.
      * Note that to be able to use R4 software, your SD card must be at most 4 GB, or have a single partition in the first 4 GB of the SD card. R4 card commands cannot address SD sectors above 4 GB!
   * `DSPICO_ENABLE_WRFUXXED` - Enables emulation of the IS-SPI-USB-ADAPTER to support the WRFUxxed exploit. This requires <code>uartBufv060.bin</code> to be placed in the `data/` folder.
   * `ENABLE_PREVENT_DSI_AUTOBOOT` - Experimental feature that prevents DSi consoles from autobooting when the autoboot flag is set. It was intended to be used with WRFU Tester, which has the autoboot flag set. It is generally not recommended to use this, as it does not work properly with the 3DS and has not been tested much.

### Setting up the rom(s)
To compile and properly use the firmware, you will need to place a valid DS rom in the `roms/` folder, named `default.nds`. Additionally, you may include a second rom in the `roms/` folder named `dsimode.nds`, if you wish to have a different rom for DS consoles and DSi/3DS consoles.

Additionally, alongside the main roms, 3DS/DSi ntrboot images can be added to the firmware by placing them in the `roms/` folder, named `ntrboot.nds` (for 3DS, or DSi if only this image) and `ntrboot.dsi` (DSi image to be used if `ntrboot.nds` is present). If any of them is present, the firmware will enable automatic detect ntrboot (this might cause issues with third party cart loading programs).
<table>
    <tr>
        <th>Usage</th>
        <th>default.nds</th>
        <th>dsimode.nds</th>
        <th>Notes</th>
    </tr>
    <tr>
        <td>Single rom for DS and/or DSi</td>
        <td>Your rom</td>
        <td>-</td>
        <td>The rom must contain NTR blowfish keys.<br>If the rom is hybrid or DSi exclusive, it must additionally contain TWL blowfish keys.</td>
    </tr>
    <tr>
        <td>Hybrid bootloader</td>
        <td>Bootloader</td>
        <td>-</td>
        <td>The bootloader rom must be patched with the DSpico DLDI.<br>The bootloader rom must contain NTR and TWL blowfish keys.</td>
    </tr>
    <tr>
        <td>DSi ntrboot</td>
        <td>GCD rom</td>
        <td>-</td>
        <td>The rom must contain GCD blowfish keys and must be properly signed.<br>It might be possible that the DSpico is not detected by the console during DSi ntrboot, if that happens there might be the need to use USB power, such that the firmware is booted before starting the DSi.<br>This will <b>only</b> serve the DSi ntrboot rom.</td>
    </tr>
    <tr>
        <td>3DS ntrboot</td>
        <td>3DS ntrboot rom</td>
        <td>-</td>
        <td>A 3DS ntrboot rom consists of a header, the blowfish keys and the firm to boot.<br>This will <b>only</b> serve the 3DS ntrboot rom.</td>
    </tr>
    <tr>
        <td>Separate rom for DS and DSi</td>
        <td>Your DS rom</td>
        <td>Your DSi rom</td>
        <td>The DS rom must contain NTR blowfish keys.<br>The DSi rom must contain both NTR and TWL blowfish keys.</td>
    </tr>
    <tr>
        <td>WRFUxxed</td>
        <td>Bootloader</td>
        <td>WRFU Tester v0.60</td>
        <td>The bootloader rom and <code>uartBufv060.bin</code> must be patched with the DSpico DLDI.<br><code>uartBufv060.bin</code> must be placed in the <code>/data</code> folder.<br>The bootloader rom must contain NTR blowfish keys.<br>The <code>DSPICO_ENABLE_WRFUXXED</code> define in <code>CMakeLists.txt</code> must be enabled.</td>
    </tr>
</table>
<table>
    <tr>
        <th>Usage</th>
        <th>ntrboot.nds</th>
        <th>ntrbootdsi.nds</th>
        <th>Notes</th>
    </tr>
    <tr>
        <td>DSi ntrboot</td>
        <td>GCD rom</td>
        <td>-</td>
        <td>The rom must be the same format as from the table above.<br>The pico will automatically detect ntrboot and serve this rom.</td>
    </tr>
    <tr>
        <td>3DS ntrboot</td>
        <td>3DS ntrboot rom</td>
        <td>-</td>
        <td>The rom must be the same format as from the table above.<br>The pico will automatically detect ntrboot and serve this rom.</td>
    </tr>
    <tr>
        <td>Separate rom for DSi and 3DS ntrboot</td>
        <td>3DS ntrboot rom</td>
        <td>GCD rom</td>
        <td>The roms must be the same format as from the table above.<br>The pico will automatically detect ntrboot and the console and serve the appropriate rom accordingly.</td>
    </tr>
</table>

## Compiling
Simply run `./compile.sh` to compile the firmware. Once it is complete, you will be able to find `DSpico.uf2` in the `build/` folder, which you can use to flash your DSpico board with.

> [!IMPORTANT]
> The firmware only works correctly when build with optimization. Recommended is `RelWithDebInfo`.

## License

The firmware for the DSpico project includes code that is licensed under the following:

SPDX-License-Identifier: Zlib
- Miscellaneous source files

SPDX-License-Identifier: BSD-3-Clause
- Pico SDK

SPDX-License-Identifier: GPL-3.0-or-later
- ZuluSCSI

For details, see the `license` directory, as well as `LICENSE.txt`.

## Contributors
- [@Gericom](https://github.com/Gericom)
- [@XLuma](https://github.com/XLuma)
- [@Dartz150](https://github.com/Dartz150)
- [@pedro-javierf](https://github.com/pedro-javierf)
