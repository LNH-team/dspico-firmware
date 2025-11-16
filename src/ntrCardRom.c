#include "common.h"
#include "hardware/dma.h"
#include "ntrCardRom.h"

ntr_rom_emu_t gNtrRomEmu;

void __scratch_y("cpu0") ntrc_dmaToBus(const void* data, u32 length)
{
    dma_channel_config c = dma_channel_get_default_config(0);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio0, 0, true));
    dma_channel_configure(0, &c,
        &pio0->txf[0], // Destination pointer
        data,          // Source pointer
        length >> 2,   // Number of transfers
        true           // Start immediately
    );
}
