#include "common.h"
#include <stdio.h>
#include "hardware/pio.h"
#include "ntrCard.pio.h"
#include "wrfuxxed.h"
#include "ntrCardSpiUart.h"

#ifdef DSPICO_ENABLE_WRFUXXED

static ntrc_spi_uart_t sState;

static void dataIrq(void)
{
    u8 data = pio0->rxf[1];

    if (sState.writeDataCnt > 0)
    {
        //data is write data, ignore
        sState.writeDataCnt--;
    }
    else if ((data & 0xF0) == NTRC_SPI_UART_CMD_ECHO_1 || (data & 0xF0) == NTRC_SPI_UART_CMD_ECHO_2)
    {
        sState.retDataBuf[0] = data;
        sState.retDataCnt = 1;
        sState.retDataPtr = 0;
    }
    else if ((data & 0xF0) == NTRC_SPI_UART_CMD_WRITE)
    {
        sState.writeDataCnt = (data & 0xF) + 1;
    }
    else if ((data & 0xF0) == NTRC_SPI_UART_CMD_READ)
    {
        int len = (data & 0xF) + 1;
        sState.retDataBuf[0] = sState.status;
        for (int i = 0; i < len; i++)
            sState.retDataBuf[1 + i] = *sState.readBufPtr++;
        sState.readBufCount -= len;
        //set read data
        sState.retDataCnt = len + 1;
        sState.retDataPtr = 0;
    }
    else
    {
        switch (data)
        {
            case NTRC_SPI_UART_CMD_RESET:
            {
                sState.status = 0x90;
                break;
            }

            case NTRC_SPI_UART_CMD_GET_WRITABLE_LENGTH:
            {
                sState.retDataBuf[0] = 0x90;
                sState.retDataBuf[1] = 0x00; //lsb
                sState.retDataBuf[2] = 0x00; //msb
                sState.retDataCnt = 3;
                sState.retDataPtr = 0;
                break;
            }

            case NTRC_SPI_UART_CMD_GET_READABLE_LENGTH:
            {
                sState.retDataBuf[0] = 0x90;
                int len = sState.readBufCount;
                if (len > 1024)
                    len = 1024;
                sState.retDataBuf[1] = len & 0xFF; //lsb
                sState.retDataBuf[2] = len >> 8; //msb
                sState.retDataCnt = 3;
                sState.retDataPtr = 0;
                break;
            }

            case NTRC_SPI_UART_CMD_INIT:
            {
                sState.retDataBuf[0] = 0x90;
                sState.retDataBuf[1] = 'S';
                sState.retDataBuf[2] = 'P';
                sState.retDataBuf[3] = 'I';
                sState.retDataBuf[4] = 'U';
                sState.retDataCnt = 5;
                sState.retDataPtr = 0;
                break;
            }
        }
    }

    if (sState.retDataCnt != 0)
    {
        pio0->txf[1] = sState.retDataBuf[sState.retDataPtr++] << 24;
        sState.retDataCnt--;
    }
    else
    {
        pio0->txf[1] = sState.status << 24;
    }
}

void ntrc_initSpiUart(u32 programOffs)
{
    sState.progOffs = programOffs;
    pio_sm_config c = ntr_card_spi_program_get_default_config(programOffs);
    sm_config_set_out_pins(&c, PIN_D6, 1);
    sm_config_set_in_pins(&c, PIN_D7);
    sm_config_set_sideset_pins(&c, PIN_D6);
    sm_config_set_out_shift(&c, false, true, 8);
    sm_config_set_in_shift(&c, false, true, 8);
    sm_config_set_clkdiv(&c, 1);
    pio_sm_set_pindirs_with_mask(pio0, 1, 0, 0xC0000);
    pio_sm_set_pins_with_mask(pio0, 1, 0, 0xC0000);
    pio_sm_init(pio0, 1, programOffs, &c);
    pio_set_irq1_source_enabled(pio0, pis_sm1_rx_fifo_not_empty, true);
    irq_set_exclusive_handler(PIO0_IRQ_1, dataIrq);
    pio_sm_set_enabled(pio0, 1, true);
    ntrc_resetSpiUart();
}

void ntrc_resetSpiUart(void)
{
    pio_sm_set_enabled(pio0, 1, false);

    sState.status = 0x80;
    sState.retDataPtr = 0;
    sState.retDataCnt = 0;
    sState.writeDataCnt = 0;
    sState.readBufPtr = gWrfuxxedPayload;
    sState.readBufCount = (int)gWrfuxxedPayloadSize;

    pio_sm_set_pindirs_with_mask(pio0, 1, 0, 0xC0000);
    pio_sm_clear_fifos(pio0, 1);
    pio_sm_restart(pio0, 1);
    pio_sm_clkdiv_restart(pio0, 1);
    irq_clear(PIO0_IRQ_1);
    irq_set_enabled(PIO0_IRQ_1, true);
    pio_sm_exec(pio0, 1, pio_encode_jmp(sState.progOffs));
    pio_sm_set_enabled(pio0, 1, true);
    pio0->txf[1] = 0x80 << 24; //put first byte
}

#endif
