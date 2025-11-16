# Supported commands
This page describes the card commands supported by the DSpico firmware. It is recommended to use at least 4 latency cycles for commands that read data (DSpico -> DS) and at least 8 latency cycles for commands that write data (DS -> DSpico). Normally a 6.7 MHz clock should be used.

## Normal mode
When the cartridge boots, or after a card reset, the cartridge is in normal mode.

<table>
    <tr>
        <th>Command</th>
        <th>Name</th>
        <th>Description</th>
    </tr>
    <tr>
        <td><code>00 xx xx xP PP xx xx xx</code></td>
        <td><code>NTR_CMD_ID_NORMAL_READ_PAGE</code></td>
        <td>Reads a rom header page (512 bytes).<br><code>PPP</code> must be a multiple of 512.</td>
    </tr>
    <tr>
        <td><code>3C xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_NORMAL_CHANGE_MODE_NTR</code></td>
        <td>Switches to NTR secure mode.</td>
    </tr>
    <tr>
        <td><code>3D xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_NORMAL_CHANGE_MODE_TWL</code></td>
        <td>Switches to TWL secure mode.</td>
    </tr>
    <tr>
        <td><code>71 xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_NORMAL_3DS_DETECT</code></td>
        <td>Ignored, but used for console type detection.</td>
    </tr>
    <tr>
        <td><code>90 xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_NORMAL_READ_ID</code></td>
        <td>Reads the card id (4 bytes).</td>
    </tr>
    <tr>
        <td><code>9F xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_NORMAL_LOAD_TABLE</code></td>
        <td>Ignored, but used for console type detection.</td>
    </tr>
</table>

## Secure mode
In secure mode the received commands are blowfish encrypted.

<table>
    <tr>
        <th></th>
        <th>NTR location</th>
        <th>TWL location</th>
    </tr>
    <tr>
        <td>P table</td>
        <td><code>0x1600</code></td>
        <td><code>twlArea + 0x600</code></td>
    </tr>
    <tr>
        <td>S boxes</td>
        <td><code>0x1C00</code></td>
        <td><code>twlArea + 0xC00</code></td>
    </tr>
    <tr>
        <td>Secure area</td>
        <td><code>0x4000</code> to <code>0x8000</code></td>
        <td><code>twlArea + 0x3000</code> to <code>twlArea + 0x7000</code></td>
    </tr>
</table>

`twlArea` is the 16-bit value at `0x92` multiplied by `0x80000`.

In secure mode all commands need to be issued at least two times. The first time is used to decrypt the command. No data is returned. The second time the command is actually executed.
<table>
    <tr>
        <th>Command</th>
        <th>Name</th>
        <th>Description</th>
    </tr>
    <tr>
        <td><code>1x xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_SECURE_READ_ID</code></td>
        <td>Reads the card id (4 bytes).</td>
    </tr>
    <tr>
        <td><code>2S SS Sx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_SECURE_READ_SEGMENT</code></td>
        <td>Reads a 4 KB secure segment.<br>Segment number <code>SSS</code> must be 4, 5, 6 or 7.<br>The command needs to be issued 9 times to get the entire segment.<br>The first time, the command is decrypted and no data is returned.<br>The next 8 times 512 bytes are returned.</td>
    </tr>
    <tr>
        <td><code>4x xx xM MM NN Nx xx xx</code></td>
        <td><code>NTR_CMD_ID_SECURE_SCRAMBLER_ON</code></td>
        <td>Enables the scrambler. <code>MMMNNN</code> is used to seed the scrambler.</td>
    </tr>
    <tr>
        <td><code>Ax xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_SECURE_CHANGE_MODE</code></td>
        <td>Switches to game mode.</td>
    </tr>
</table>

## Game mode
Game mode is the mode that is used while a DS application is running. Blowfish is no longer used and commands only need to be issued once.

<table>
    <tr>
        <th>Command</th>
        <th>Name</th>
        <th>Description</th>
    </tr>
    <tr>
        <td><code>B7 PP PP PP PP xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_READ_PAGE</code></td>
        <td>Reads a rom page (512 bytes).<br><code>PPPPPPPP</code> must be a multiple of 512.</td>
    </tr>
    <tr>
        <td><code>B8 xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_READ_ID</code></td>
        <td>Reads the card id (4 bytes).</td>
    </tr>
    <tr>
        <td><code>FC xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_DISABLE_SCRAMBLING</code></td>
        <td>Switches to unscrambled game mode.</td>
    </tr>
</table>

## Unscrambled game mode
In this mode no scrambling is used and the extended features of the DSpico are available. On the DS side it is recommended to set the scrambler seeds to zero, such that no scrambling is used no matter if scrambling is enabled or disabled in the control register (when the seed is zero, the scrambler only outputs zeros). By doing this, R4 specific applications will automatically not have scrambling.

<table>
    <tr>
        <th>Command</th>
        <th>Name</th>
        <th>Description</th>
    </tr>
    <tr>
        <td><code>B7&nbsp;PP&nbsp;PP&nbsp;PP&nbsp;PP&nbsp;xx&nbsp;xx&nbsp;xx</code></td>
        <td><code>NTR_CMD_ID_GAME_READ_PAGE</code></td>
        <td>Returns 512 bytes of garbage unless the <code>ENABLE_R4_MODE</code> define is used (see below).</td>
    </tr>
    <tr>
        <td><code>B8 xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_READ_ID</code></td>
        <td>Reads the card id (4 bytes).</td>
    </tr>
    <tr>
        <td><code>E3 xx xx xx SS SS SS SS</code></td>
        <td><code>NTR_CMD_ID_GAME_REQ_SD_READ</code></td>
        <td>Requests an SD read of sector <code>SSSSSSSS</code>.</td>
    </tr>
    <tr>
        <td><code>E4 xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_GET_SD_STAT</code></td>
        <td>Returns 1 when the current SD operation (read or write) is complete,<br>or 0 otherwise. (4 bytes)</td>
    </tr>
    <tr>
        <td><code>E5 xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_GET_SD_DATA</code></td>
        <td>Returns 512 bytes of SD data that was read previously.</td>
    </tr>
    <tr>
        <td><code>E8 SS xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_USB_COMMAND</code></td>
        <td>Performs a USB command. This command has no payload data.<br>See below for the list of sub commands (<code>SS</code>).</td>
    </tr>
    <tr>
        <td><code>E9 xI PP xT LL LL LL LL</code></td>
        <td><code>NTR_CMD_ID_GAME_USB_WRITE_DATA</code></td>
        <td>Writes 512 bytes of data to an endpoint buffer.<br><code>PP</code> is the endpoint id. <code>I</code> specifies the buffer index (0 or 1).<br><code>T</code> specifies whether a transfer should be started (1) or not (0).<br><code>LLLLLLLL</code> specifies the actual data length (0 <= length <= 512).<br>Length zero is special cased to expect no payload data, for zero length transfers.</td>
    </tr>
    <tr>
        <td><code>EA xx PP xI xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_USB_READ_DATA</code></td>
        <td>Reads 512 bytes of data from an endpoint buffer.<br><code>PP</code> is the endpoint id. <code>I</code> specifies the buffer index (0 or 1).</td>
    </tr>
    <tr>
        <td><code>EB xx xx xx xx xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_USB_GET_EVENT</code></td>
        <td>Dequeues an event from the USB event queue and returns it (4 bytes).</td>
    </tr>
    <tr>
        <td><code>F6 E1 0D 9Q SS SS SS SS</code></td>
        <td><code>NTR_CMD_ID_GAME_WRITE_SD_DATA</code></td>
        <td>Writes 512 bytes of SD sector data. <code>Q</code> is <code>10XY</code> (binary) with<br><code>X</code> the <code>WRITE_SD_DATA_IS_FIRST_FLAG</code> indicating the start of a sequential write, and<br><code>Y</code> the <code>WRITE_SD_DATA_IS_LAST_FLAG</code> indicating the last sector of a sequential write.</td>
    </tr>
</table>

### SD read
This pseudocode illustrates how to do a (sequential) SD read. When a sector read is requested, the DSpico will automatically start reading the next sector when the read of the first sector completes. This improves throughput, as the completed sector can be transferred to the DS, while the next sector is being read from the SD card.
```C
read(sector, count, data)
{
    NTR_CMD_ID_GAME_REQ_SD_READ(sector);
    do
    {
        while (NTR_CMD_ID_GAME_GET_SD_STAT() == 0);
        NTR_CMD_ID_GAME_GET_SD_DATA(data);
    } while (--count != 0);

    while (NTR_CMD_ID_GAME_GET_SD_STAT() == 0);
}
```

### SD write
This pseudocode illustrates how to do a (sequential) SD write. While the DSpico is writing a sector to the SD card, the next sector can already be transferred to the DSpico to improve throughput.
```C
write(sector, count, data)
{
    if (count == 1)
    {
        NTR_CMD_ID_GAME_WRITE_SD_DATA(sector, first: true, last: true, data); // send 0 = last
    }
    else if (count > 1)
    {
        NTR_CMD_ID_GAME_WRITE_SD_DATA(sector, first: true, last: false, data); // send 0
        sector++;
        for (i = 1; i < count - 1; i++)
        {
            NTR_CMD_ID_GAME_WRITE_SD_DATA(sector, first: false, last: false, data); // send i
            sector++;
            while (NTR_CMD_ID_GAME_GET_SD_STAT() == 0); // wait i - 1
        }

        NTR_CMD_ID_GAME_WRITE_SD_DATA(sector, first: false, last: true, data); // send last
        while (NTR_CMD_ID_GAME_GET_SD_STAT() == 0); // wait last - 1
    }

    while (NTR_CMD_ID_GAME_GET_SD_STAT() == 0); // wait last
}
```

### NTR_CMD_ID_GAME_USB_COMMAND sub commands
Most of the sub commands directly map to tinyusb functions.
<table>
    <tr>
        <th>Command</th>
        <th>Name</th>
        <th>Description</th>
    </tr>
    <tr>
        <td><code>E8 01 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_INIT</code></td>
        <td>Initializes USB.</td>
    </tr>
    <tr>
        <td><code>E8 02 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_BEGIN_SET_ADDRESS</code></td>
        <td><code>dcd_set_address(0, 0)</code></td>
    </tr>
    <tr>
        <td><code>E8 03 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_REMOTE_WAKEUP</code></td>
        <td><code>dcd_remote_wakeup(0)</code></td>
    </tr>
    <tr>
        <td><code>E8 04 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_CONNECT</code></td>
        <td><code>dcd_connect(0)</code></td>
    </tr>
    <tr>
        <td><code>E8 05 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_DISCONNECT</code></td>
        <td><code>dcd_disconnect(0)</code></td>
    </tr>
    <tr>
        <td><code>E8 06 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_SOF_ENABLE</code></td>
        <td><code>dcd_sof_enable(0, true)</code></td>
    </tr>
    <tr>
        <td><code>E8 07 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_SOF_DISABLE</code></td>
        <td><code>dcd_sof_enable(0, false)</code></td>
    </tr>
    <tr>
        <td><code>E8 08 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_EP_CLOSE_ALL</code></td>
        <td><code>dcd_edpt_close_all(0)</code></td>
    </tr>
    <tr>
        <td><code>E8 09 PP xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_EP_STALL</code></td>
        <td><code>dcd_edpt_stall(0, PP)</code></td>
    </tr>
    <tr>
        <td><code>E8 0A PP xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_EP_CLEAR_STALL</code></td>
        <td><code>dcd_edpt_clear_stall(0, PP)</code></td>
    </tr>
    <tr>
        <td><code>E8 0B xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_EP_CLOSE</code></td>
        <td><code>dcd_edpt_close(0, PP)</code></td>
    </tr>
    <tr>
        <td><code>E8 0C RR xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_FINISH_SET_ADDRESS</code></td>
        <td><code>usb_hw->dev_addr_ctrl = RR</code></td>
    </tr>
    <tr>
        <td><code>E8 0D PP xT xx xx xS SS</code></td>
        <td><code>USB_SUB_COMMAND_EP_OPEN</code></td>
        <td>Opens endpoint <code>PP</code>. The endpoint will have type <code>T</code><br>(0 = control, 1 = isochronous, 2 = bulk, 3 = interrupt)<br>and maximum packet size <code>SSS</code>.</td>
    </tr>
    <tr>
        <td><code>E8 0E xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_CLEAR_EVENT_QUEUE</code></td>
        <td>Clears the USB event queue.</td>
    </tr>
    <tr>
        <td><code>E8 0F xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_DEINIT</code></td>
        <td>Deinitializes USB.</td>
    </tr>
    <tr>
        <td><code>E8 10 PP xI LL LL LL LL</code></td>
        <td><code>USB_SUB_COMMAND_BEGIN_TRANSFER</code></td>
        <td>Starts a transfer on endpoint <code>PP</code>. <code>I</code> is the buffer index (0 or 1),<br>and <code>LLLLLLLL</code> is the length (0 <= length <= 512).</td>
    </tr>
    <tr>
        <td><code>E8 11 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_INTERRUPT_ENABLE</code></td>
        <td><code>dcd_int_enable(0)</code></td>
    </tr>
    <tr>
        <td><code>E8 12 xx xx xx xx xx xx</code></td>
        <td><code>USB_SUB_COMMAND_INTERRUPT_DISABLE</code></td>
        <td><code>dcd_int_disable(0)</code></td>
    </tr>
</table>

### R4 emulation
The following commands are additionally supported for R4 emulation when the `ENABLE_R4_MODE` define is used. See also [r4.md](r4.md).
<table>
    <tr>
        <th>Command</th>
        <th>Name</th>
        <th>Description</th>
    </tr>
    <tr>
        <td><code>00&nbsp;00&nbsp;00&nbsp;00&nbsp;xx&nbsp;xx&nbsp;xx&nbsp;xx</code></td>
        <td>-</td>
        <td>Returns zero (4 bytes).</td>
    </tr>
    <tr>
        <td><code>B0 xx xx xx xx xx xx xx</code></td>
        <td>-</td>
        <td>Get R4 card info. Returns 0x1F4 (4 bytes).</td>
    </tr>
    <tr>
        <td><code>B2 PP PP PP PP xx xx xx</code></td>
        <td>-</td>
        <td>Starts a save read at address <code>PPPPPPPP</code>. The address must be 512 byte aligned. The command returns 1 while the card is busy reading, and 0 once the read is complete (4 bytes).</td>
    </tr>
    <tr>
        <td><code>B3 xx xx xx xx xx xx xx</code></td>
        <td>-</td>
        <td>Returns the 512 bytes of save data that were previously requested with command <code>B2</code>.</td>
    </tr>
    <tr>
        <td><code>B4 PP PP PP PP xx xx xx</code></td>
        <td>-</td>
        <td>Initializes the cluster map for a rom (lsb of address is 0) or save file (lsb of address is 1). <code>PPPPPPPP</code> is the 2-byte aligned SD address of the FAT entry of the file. The command returns 1 while the cluster map is being initialized,<br>and 0 once complete (4 bytes).</td>
    </tr>
    <tr>
        <td><code>B6 PP PP PP PP xx xx xx</code></td>
        <td>-</td>
        <td>Starts a rom read at address <code>PPPPPPPP</code>. The address must be 512 byte aligned. The command returns 1 while the card is busy reading, and 0 once the read is complete (4 bytes).</td>
    </tr>
    <tr>
        <td><code>B7 PP PP PP PP xx xx xx</code></td>
        <td><code>NTR_CMD_ID_GAME_READ_PAGE</code></td>
        <td>Returns the 512 bytes of rom data that were previously requested with command <code>B6</code>. <code>PPPPPPPP</code> is the 512 byte aligned rom address.</td>
    </tr>
    <tr>
        <td><code>B9 PP PP PP PP xx xx xx</code></td>
        <td>-</td>
        <td>Starts an SD read at SD address <code>PPPPPPPP</code>. The address must be 512 byte aligned. The command returns 0x1F4 while the card is busy reading, and 0 once the read is complete (4 bytes).</td>
    </tr>
    <tr>
        <td><code>BA xx xx xx xx xx xx xx</code></td>
        <td>-</td>
        <td>Returns the 512 bytes of SD data that were previously requested with command <code>B9</code>.</td>
    </tr>
    <tr>
        <td><code>BB PP PP PP PP xx xx xx</code></td>
        <td>-</td>
        <td>Starts an SD write to SD address <code>PPPPPPPP</code>. The address must be 512 byte aligned. The command expects 512 bytes of data to write.</td>
    </tr>
    <tr>
        <td><code>BE xx xx xx xx xx xx xx</code></td>
        <td>-</td>
        <td>Gets the status of the SD write that was started previously with command <code>BB</code>. The command returns 1 while the card is busy writing, and 0 once the write is complete (4 bytes).</td>
    </tr>
</table>
