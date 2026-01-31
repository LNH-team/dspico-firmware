.section .flashdata,"a"

.global gDefaultRom
gDefaultRom:
    .incbin "../roms/default.nds"

gDefaultRomDataEnd:
.global gDefaultRomSize
.equ gDefaultRomSize, gDefaultRomDataEnd - gDefaultRom

#if defined(DETECT_CONSOLE_TYPE)
.global gDsiRom
gDsiRom:
    .incbin "../roms/dsimode.nds"

gDsiRomDataEnd:
.global gDsiRomSize
.equ gDsiRomSize, gDsiRomDataEnd - gDsiRom
#endif

#if defined(ENABLE_NTRBOOT_AUTO_DETECTION)
.global gNtrbootRom
gNtrbootRom:
    .incbin "../roms/ntrboot.nds"

gNtrbootRomEnd:
.global gNtrbootRomSize
.equ gNtrbootRomSize, gNtrbootRomEnd - gNtrbootRom

#if defined(ENABLE_NTRBOOT_CONSOLE_TYPE_DETECTION)
.global gNtrbootDsiRom
gNtrbootDsiRom:
    .incbin "../roms/ntrbootdsi.nds"

gNtrbootDsiRomEnd:
.global gNtrbootDsiRomSize
.equ gNtrbootDsiRomSize, gNtrbootDsiRomEnd - gNtrbootDsiRom
#endif
#endif
.end
