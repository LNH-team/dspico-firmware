.section .flashdata,"a"

#if defined(DSPICO_ENABLE_WRFUXXED)
.global gWrfuxxedPayload
gWrfuxxedPayload:
    .incbin "../data/uartBufv060.bin"

gWrfuxxedPayloadDataEnd:
.global gWrfuxxedPayloadSize
.equ gWrfuxxedPayloadSize, gWrfuxxedPayloadDataEnd - gWrfuxxedPayload
#endif
.end