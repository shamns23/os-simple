; ملف معالجات المقاطعات بلغة التجميع
; مستوحى من Linux kernel 0.01

[BITS 32]

; تصدير الرموز للاستخدام في C
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8
global isr10, isr11, isr12, isr13, isr14
global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global load_idt

; استيراد معالجات C
extern isr_handler
extern irq_handler

section .text

; ماكرو لمعالجات الاستثناءات بدون رمز خطأ
%macro ISR_NOERRCODE 1
isr%1:
    cli                     ; تعطيل المقاطعات
    push byte 0             ; دفع رمز خطأ وهمي
    push byte %1            ; دفع رقم المقاطعة
    jmp isr_common_stub     ; القفز للمعالج المشترك
%endmacro

; ماكرو لمعالجات الاستثناءات مع رمز خطأ
%macro ISR_ERRCODE 1
isr%1:
    cli                     ; تعطيل المقاطعات
    push byte %1            ; دفع رقم المقاطعة
    jmp isr_common_stub     ; القفز للمعالج المشترك
%endmacro

; ماكرو لمعالجات المقاطعات الخارجية
%macro IRQ 2
irq%1:
    cli                     ; تعطيل المقاطعات
    push byte 0             ; دفع رمز خطأ وهمي
    push byte %2            ; دفع رقم المقاطعة
    jmp irq_common_stub     ; القفز للمعالج المشترك
%endmacro

; تعريف معالجات الاستثناءات
ISR_NOERRCODE 0     ; Division By Zero Exception
ISR_NOERRCODE 1     ; Debug Exception
ISR_NOERRCODE 2     ; Non Maskable Interrupt Exception
ISR_NOERRCODE 3     ; Breakpoint Exception
ISR_NOERRCODE 4     ; Into Detected Overflow Exception
ISR_NOERRCODE 5     ; Out of Bounds Exception
ISR_NOERRCODE 6     ; Invalid Opcode Exception
ISR_NOERRCODE 7     ; No Coprocessor Exception
ISR_ERRCODE   8     ; Double Fault Exception
ISR_ERRCODE   10    ; Bad TSS Exception
ISR_ERRCODE   11    ; Segment Not Present Exception
ISR_ERRCODE   12    ; Stack Fault Exception
ISR_ERRCODE   13    ; General Protection Fault Exception
ISR_ERRCODE   14    ; Page Fault Exception

; تعريف معالجات المقاطعات الخارجية
IRQ 0, 32           ; Timer
IRQ 1, 33           ; Keyboard
IRQ 2, 34           ; Cascade (used internally by the two PICs. never raised)
IRQ 3, 35           ; COM2
IRQ 4, 36           ; COM1
IRQ 5, 37           ; LPT2
IRQ 6, 38           ; Floppy Disk
IRQ 7, 39           ; LPT1

; المعالج المشترك للاستثناءات
isr_common_stub:
    pusha               ; حفظ جميع السجلات العامة
    
    mov ax, ds          ; حفظ segment descriptor السفلي
    push eax
    
    mov ax, 0x10        ; تحميل kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call isr_handler    ; استدعاء معالج C
    
    pop eax             ; استعادة segment descriptor الأصلي
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                ; استعادة جميع السجلات
    add esp, 8          ; تنظيف رقم المقاطعة ورمز الخطأ
    sti                 ; تفعيل المقاطعات
    iret                ; العودة من المقاطعة

; المعالج المشترك للمقاطعات الخارجية
irq_common_stub:
    pusha               ; حفظ جميع السجلات العامة
    
    mov ax, ds          ; حفظ segment descriptor السفلي
    push eax
    
    mov ax, 0x10        ; تحميل kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call irq_handler    ; استدعاء معالج C
    
    pop eax             ; استعادة segment descriptor الأصلي
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa                ; استعادة جميع السجلات
    add esp, 8          ; تنظيف رقم المقاطعة ورمز الخطأ
    sti                 ; تفعيل المقاطعات
    iret                ; العودة من المقاطعة

; دالة تحميل IDT
load_idt:
    mov eax, [esp+4]    ; الحصول على مؤشر IDT من المعامل
    lidt [eax]          ; تحميل IDT
    ret                 ; العودة
