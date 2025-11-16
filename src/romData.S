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
.end
