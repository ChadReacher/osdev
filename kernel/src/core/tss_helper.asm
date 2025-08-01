global tss_flush
tss_flush:
    ; ((5 (fifth entry in GDT) * 8 (size)) | 3) = 43 / 0x2B
    mov ax, 0x2B
    ltr ax
    ret
