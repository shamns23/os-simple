[BITS 16]
[ORG 0x7C00]

start:
    ; إعداد المقاطع
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; مسح الشاشة
    call clear_screen
    
    ; طباعة رسالة "hi"
    mov si, hi_msg
    call print_string
    
    ; حلقة لا نهائية للحفاظ على تشغيل النظام
    jmp $
    
clear_screen:
    ; تعيين وضع الفيديو لمسح الشاشة
    mov ah, 0x00        ; وظيفة تعيين وضع الفيديو
    mov al, 0x03        ; وضع النص 80x25 ملون
    int 0x10            ; استدعاء BIOS
    ret

print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string
done:
    ret
    
hi_msg db 'hi', 13, 10, 0

; ملء باقي القطاع بالأصفار
times 510-($-$$) db 0
dw 0xAA55               ; توقيع Boot sector