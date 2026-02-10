#include "../common.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "rp2040_sdio.h"
#include "SdCardInfo.h"
#include "SdCard.h"

#define SEQUENTIAL_READ_TIMEOUT_MICROSECONDS    1000000 // 1 second

sdio_status_t SdCard::Cmd0GoIdleState() const
{
    return rp2040_sdio_command_R1(CMD0, 0, NULL); // GO_IDLE_STATE
}

sdio_status_t SdCard::Cmd2AllSendCid(cid_t& cid) const
{
    return rp2040_sdio_command_R2(CMD2, 0, (u8*)&cid); // ALL_SEND_CID
}

sdio_status_t SdCard::Cmd3SendRelativeAddr(u32& rca) const
{
    return rp2040_sdio_command_R1(CMD3, 0, &rca);
}

sdio_status_t SdCard::Cmd7SelectCard(u32 rca) const
{
    u32 response;
    return rp2040_sdio_command_R1(CMD7, rca, &response);
}

sdio_status_t SdCard::Cmd8SendIfCond(u32 argument, u32& response) const
{
    return rp2040_sdio_command_R1(CMD8, argument, &response); // SEND_IF_COND
}

sdio_status_t SdCard::Cmd9SendCsd(u32 rca, csd_t& csd) const
{
    return rp2040_sdio_command_R2(CMD9, rca, (u8*)&csd);
}

sdio_status_t SdCard::Cmd12StopTransmission() const
{
    u32 response;
    return rp2040_sdio_command_R1(CMD12, 0, &response);
}

sdio_status_t SdCard::Cmd16SetBlocklen(u32 blockLength) const
{
    u32 response;
    return rp2040_sdio_command_R1(16, blockLength, &response);
}

sdio_status_t SdCard::Cmd17ReadSingleBlock(u32 address) const
{
    u32 response;
    return rp2040_sdio_command_R1(CMD17, address, &response);
}

sdio_status_t SdCard::Cmd18ReadMultipleBlock(u32 address) const
{
    u32 response;
    return rp2040_sdio_command_R1(CMD18, address, &response);
}

sdio_status_t SdCard::Cmd24WriteBlock(u32 address) const
{
    u32 response;
    return rp2040_sdio_command_R1(CMD24, address, &response);
}

sdio_status_t SdCard::Cmd25WriteMultipleBlock(u32 address) const
{
    u32 response;
    return rp2040_sdio_command_R1(CMD25, address, &response);
}

sdio_status_t SdCard::Cmd55AppCmd(u32 rca) const
{
    u32 response;
    return rp2040_sdio_command_R1(CMD55, rca, &response); // APP_CMD
}

sdio_status_t SdCard::ACmd6SetBusWidth(u32 argument) const
{
    u32 response;
    return rp2040_sdio_command_R1(ACMD6, argument, &response);
}

sdio_status_t SdCard::ACmd23SetWrBlkEraseCount(u32 count) const
{
    u32 response;
    return rp2040_sdio_command_R1(ACMD23, count, &response);
}

sdio_status_t SdCard::ACmd41SdSendOpCond(u32 argument, u32& response) const
{
    return rp2040_sdio_command_R3(ACMD41, argument, &response); // SD_SEND_OP_COND
}

sdio_status_t SdCard::ACmd42SetClrCardDetect(u32 argument) const
{
    u32 response;
    return rp2040_sdio_command_R1(ACMD42, argument, &response); // SET_CLR_CARD_DETECT
}

bool SdCard::TryInitialize()
{
    u32 reply;
    sdio_status_t status;
    
    // Initialize at ~403.225 kHz clock speed
    rp2040_sdio_init(62);

    // Establish initial connection with the card
    for (int retries = 0; retries < 5; retries++)
    {
        // sleep_us(1000);
        reply = 0;
        Cmd0GoIdleState();
        status = Cmd8SendIfCond(0x1AA, reply);

        if (status == SDIO_OK && reply == 0x1AA)
        {
            break;
        }
    }

    if (reply != 0x1AA || status != SDIO_OK)
    {
        return false;
    }

    // Send ACMD41 to begin card initialization and wait for it to complete
    u32 start = millis();
    do
    {
        if (Cmd55AppCmd(0) != SDIO_OK ||
            ACmd41SdSendOpCond(0x50100000, _sdioOcr) != SDIO_OK) // 3.2-3.3V voltage
            // !checkReturnOk(rp2040_sdio_command_R1(ACMD41, 0xC0100000, &g_sdio_ocr)))
        {
            return false;
        }

        if ((u32)(millis() - start) > 1000)
        {
            return false;
        }
    } while (!(_sdioOcr & (1 << 31)));

    // Get CID
    if (Cmd2AllSendCid(_sdioCid) != SDIO_OK)
    {
        return false;
    }

    // Get relative card address
    if (Cmd3SendRelativeAddr(_sdioRca) != SDIO_OK)
    {
        return false;
    }

    // Get CSD
    if (Cmd9SendCsd(_sdioRca, _sdioCsd) != SDIO_OK)
    {
        return false;
    }

    // Select card
    if (Cmd7SelectCard(_sdioRca) != SDIO_OK)
    {
        return false;
    }

    // Disable DAT3 pull-up.
    if (Cmd55AppCmd(_sdioRca) != SDIO_OK ||
        ACmd42SetClrCardDetect(0) != SDIO_OK)
    {
        return false;
    }

    // Set 4-bit bus mode
    if (Cmd55AppCmd(_sdioRca) != SDIO_OK ||
        ACmd6SetBusWidth(2) != SDIO_OK)
    {
        return false;
    }

    // Increase to 25 MHz clock rate
    rp2040_sdio_init(1);

    _lastSdSector = CalculateSdCapacity() - 1;

    irq_set_exclusive_handler(TIMER_IRQ_0, []
    {
        hw_clear_bits(&timer_hw->intr, TIMER_INTR_ALARM_0_BITS);
        gSdCard.NotifySequentialReadAlarm();
    });

    _state = State::Idle;

    return true;
}

void SdCard::Update()
{
    while (true)
    {
        switch (_state)
        {
            case State::Idle:
            {
                if (_stopSequentialRead)
                {
                    _stopSequentialRead = false;
                    if (_sequentialState == SequentialState::SequentialRead)
                    {
                        StopSequentialReadWrite();
                    }
                }
                return;
            }
            case State::ReadBegin:
            {
                StateReadBegin();
                break;
            }
            case State::ReadBusy:
            {
                StateReadBusy();
                break;
            }
            case State::ReadWriteCancel:
            {
                StateReadWriteCancel();
                break;
            }
            case State::WriteBegin:
            {
                StateWriteBegin();
                break;
            }
            case State::WriteBusy:
            {
                StateWriteBusy();
                break;
            }
            case State::Uninitialized:
            default:
            {
                return;
            }
        }
    }
}

void SdCard::StateReadBegin()
{
    u32 sectorsLeft = _sectorCount - _sectorsCompleted;
    u32 startSector = _sectorAddress + _sectorsCompleted;
    if (startSector > _lastSdSector)
    {
        _state = State::Idle;
        return;
    }

    if ((u64)startSector + sectorsLeft - 1 > _lastSdSector)
    {
        sectorsLeft = _lastSdSector - startSector + 1;
    }

    if (sectorsLeft == 0)
    {
        _state = State::Idle;
        return;
    }

    u32 sdAddress = startSector;
    if (!IsSdhcCard())
    {
        sdAddress <<= 9; // for non-hc sd cards it's a byte address
    }
    
    if (_cancelRequested)
    {
        _state = State::Idle;
        return;
    }

    bool started = false;
    if (_sequentialState == SequentialState::SequentialRead &&
        _nextSequentialSector == startSector)
    {
        StopSequentialReadAlarm();
        rp2040_sdio_rx_continue(_buffer + _sectorsCompleted * 512, sectorsLeft);
        started = true;
    }
    else
    {
        if (_sequentialState != SequentialState::None)
        {
            StopSequentialReadWrite();
        }

        rp2040_sdio_rx_start(_buffer + _sectorsCompleted * 512, sectorsLeft);

        if (_cancelRequested)
        {
            rp2040_sdio_stop();
            _state = State::Idle;
            return;
        }

        _doStopTransmission = true;
        while (!_cancelRequested)
        {
            if (Cmd18ReadMultipleBlock(sdAddress) == SDIO_OK)
            {
                started = true;
                break;
            }
        }
    }

    if (_cancelRequested && started)
    {
        _state = State::ReadWriteCancel;
    }
    else
    {
        _state = State::ReadBusy;
    }
}

void SdCard::StateReadBusy()
{
    if (_cancelRequested)
    {
        _state = State::ReadWriteCancel;
        return;
    }
    auto blockStatus = rp2040_sdio_rx_poll_one_block();
    switch (blockStatus)
    {
        case SDIO_BLOCK_CRC_FAIL:
        case SDIO_BLOCK_TIMEOUT:
        {
            StopSequentialReadWrite();
            _state = State::ReadBegin; // restart from the failed sector
            break;
        }
        case SDIO_BLOCK_OK:
        {
            _sectorsCompleted++;
            break;
        }
        case SDIO_BLOCK_ALL_DONE:
        {
            _sectorsCompleted = _sectorCount;
            _nextSequentialSector = _sectorAddress + _sectorsCompleted;
            _sequentialState = SequentialState::SequentialRead;
            StartSequentialReadAlarm();
            _state = State::Idle;
            break;
        }
        case SDIO_BLOCK_NOT_READY:
        default:
        {
            __wfi();
            break;
        }
    }
}

void SdCard::StateReadWriteCancel()
{
    StopSequentialReadWrite();
    _cancelRequested = false;
    _state = State::Idle;
}

void SdCard::StateWriteBegin()
{
    _writeOffset = _sectorsCompleted;
    u32 sectorsLeft = _sectorCount - _sectorsCompleted;
    u32 startSector = _sectorAddress + _sectorsCompleted;
    if (startSector > _lastSdSector)
    {
        _state = State::Idle;
        return;
    }

    if ((u64)startSector + sectorsLeft - 1 > _lastSdSector)
    {
        sectorsLeft = _lastSdSector - startSector + 1;
    }

    if (sectorsLeft == 0)
    {
        _state = State::Idle;
        return;
    }

    u32 sdAddress = startSector;
    if (!IsSdhcCard())
    {
        sdAddress <<= 9; // for non-hc sd cards it's a byte address
    }

    if (_cancelRequested)
    {
        _state = State::Idle;
        return;
    }

    if (_sequentialState == SequentialState::SequentialWrite &&
        _nextSequentialSector == startSector)
    {
        StopSequentialReadAlarm();
    }
    else
    {
        if (_sequentialState != SequentialState::None)
        {
            StopSequentialReadWrite();
        }

        _doStopTransmission = true;
        bool started = false;
        while (!_cancelRequested)
        {
            if (Cmd25WriteMultipleBlock(sdAddress) == SDIO_OK)
            {
                started = true;
                break;
            }
        }

        if (_cancelRequested)
        {
            if (started)
                _state = State::ReadWriteCancel;
            else
                _state = State::Idle;
            return;
        }
    }

    rp2040_sdio_tx_start(_buffer + _sectorsCompleted * 512, sectorsLeft);

    if (_cancelRequested)
        _state = State::ReadWriteCancel;
    else
        _state = State::WriteBusy;
}

void SdCard::StateWriteBusy()
{
    if (_cancelRequested)
    {
        _state = State::ReadWriteCancel;
        return;
    }

    u32 bytesCompleted = 0;
    auto status = rp2040_sdio_tx_poll(&bytesCompleted);
    switch (status)
    {
        case SDIO_BUSY:
        {
            _sectorsCompleted = _writeOffset + (bytesCompleted >> 9);
            break;
        }
        case SDIO_OK:
        {
            _sectorsCompleted = _sectorCount;
            _nextSequentialSector = _sectorAddress + _sectorsCompleted;
            if (_keepSequentialWriteOpen)
            {
                _sequentialState = SequentialState::SequentialWrite;
            }
            else
            {
                StopSequentialReadWrite();
            }
            _state = State::Idle;
            break;
        }
        default:
        {
            // bad things, retry
            StopSequentialReadWrite();
            _state = State::WriteBegin; // restart from the failed sector
            break;
        }
    }
}

void SdCard::StopSequentialReadWrite()
{
    StopSequentialReadAlarm();
    rp2040_sdio_stop();
    if (_doStopTransmission)
    {
        while (Cmd12StopTransmission() != SDIO_OK);
        while (IsCardBusy());
    }
    _sequentialState = SequentialState::None;
    _stopSequentialRead = false;
}

void SdCard::StartSequentialReadAlarm()
{
    _stopSequentialRead = false;
    irq_set_enabled(TIMER_IRQ_0, false);
    hw_set_bits(&clocks_hw->sleep_en1, CLOCKS_SLEEP_EN1_CLK_SYS_TIMER_BITS);
    hw_set_bits(&clocks_hw->wake_en1, CLOCKS_WAKE_EN1_CLK_SYS_TIMER_BITS);
    hw_set_bits(&timer_hw->armed, 1);
    hw_clear_bits(&timer_hw->intr, TIMER_INTR_ALARM_0_BITS);
    hw_set_bits(&timer_hw->inte, TIMER_INTE_ALARM_0_BITS);
    irq_set_enabled(TIMER_IRQ_0, true);
    u64 target = timer_hw->timerawl + SEQUENTIAL_READ_TIMEOUT_MICROSECONDS;
    timer_hw->alarm[0] = (u32)target;
}

void SdCard::StopSequentialReadAlarm()
{
    irq_set_enabled(TIMER_IRQ_0, false);
    hw_clear_bits(&clocks_hw->sleep_en1, CLOCKS_SLEEP_EN1_CLK_SYS_TIMER_BITS);
    hw_clear_bits(&clocks_hw->wake_en1, CLOCKS_WAKE_EN1_CLK_SYS_TIMER_BITS);
}

u32 SdCard::CalculateSdCapacity() const
{
    if (_sdioCsd.v1.csd_ver == 0)
    {
        u16 cSize = (_sdioCsd.v1.c_size_high << 10) | (_sdioCsd.v1.c_size_mid << 2) | _sdioCsd.v1.c_size_low;
        u8 cSizeMult = (_sdioCsd.v1.c_size_mult_high << 1) | _sdioCsd.v1.c_size_mult_low;
        return (u32)(cSize + 1) << (cSizeMult + _sdioCsd.v1.read_bl_len - 7);
    }
    else if (_sdioCsd.v2.csd_ver == 1)
    {
        return (((u32)_sdioCsd.v2.c_size_high << 16) + ((u16)_sdioCsd.v2.c_size_mid << 8) + _sdioCsd.v2.c_size_low + 1) << 10;
    }
    else
    {
        return 0;
    }
}
