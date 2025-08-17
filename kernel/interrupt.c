#include "interrupt.h"
#include "kernel.h"
#include <stdint.h>
#include "task.h"

// جدول وصف المقاطعات ومؤشره
idt_entry_t idt[IDT_SIZE];
idt_ptr_t idt_ptr;

// مصفوفة معالجات المقاطعات
interrupt_handler_t interrupt_handlers[IDT_SIZE];

// عداد المقاطعات للإحصائيات
static uint32_t interrupt_count = 0;
static uint32_t timer_ticks = 0;

// دالة تهيئة نظام المقاطعات الكامل
void init_interrupts() {
    print_string("[KERNEL] تهيئة نظام المقاطعات...\n");
    
    // تهيئة جدول المعالجات
    for (int i = 0; i < IDT_SIZE; i++) {
        interrupt_handlers[i] = NULL;
    }
    
    // تهيئة IDT
    init_idt();
    
    // تهيئة PIC
    init_pic();
    
    // تسجيل معالجات الاستثناءات الأساسية
    register_interrupt_handler(EXCEPTION_DIVIDE_ERROR, divide_error_handler);
    register_interrupt_handler(EXCEPTION_PAGE_FAULT, page_fault_handler);
    register_interrupt_handler(EXCEPTION_GENERAL_PROTECTION, general_protection_fault_handler);
    
    // تسجيل معالجات المقاطعات الخارجية
    register_interrupt_handler(IRQ_TIMER, timer_handler);
    register_interrupt_handler(IRQ_KEYBOARD, keyboard_handler);
    
    // تحميل IDT
    load_idt();
    
    print_string("[KERNEL] تم تهيئة نظام المقاطعات بنجاح\n");
}

// دالة تهيئة IDT
void init_idt() {
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_SIZE) - 1;
    idt_ptr.base = (uintptr_t)&idt;
    
    // مسح جدول IDT
    for (int i = 0; i < IDT_SIZE; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }
    
    // تعيين معالجات الاستثناءات (ISR)
    set_idt_entry(0, (uintptr_t)isr0, 0x08, INTERRUPT_GATE);
    set_idt_entry(1, (uintptr_t)isr1, 0x08, INTERRUPT_GATE);
    set_idt_entry(2, (uintptr_t)isr2, 0x08, INTERRUPT_GATE);
    set_idt_entry(3, (uintptr_t)isr3, 0x08, INTERRUPT_GATE);
    set_idt_entry(4, (uintptr_t)isr4, 0x08, INTERRUPT_GATE);
    set_idt_entry(5, (uintptr_t)isr5, 0x08, INTERRUPT_GATE);
    set_idt_entry(6, (uintptr_t)isr6, 0x08, INTERRUPT_GATE);
    set_idt_entry(7, (uintptr_t)isr7, 0x08, INTERRUPT_GATE);
    set_idt_entry(8, (uintptr_t)isr8, 0x08, INTERRUPT_GATE);
    set_idt_entry(10, (uintptr_t)isr10, 0x08, INTERRUPT_GATE);
    set_idt_entry(11, (uintptr_t)isr11, 0x08, INTERRUPT_GATE);
    set_idt_entry(12, (uintptr_t)isr12, 0x08, INTERRUPT_GATE);
    set_idt_entry(13, (uintptr_t)isr13, 0x08, INTERRUPT_GATE);
    set_idt_entry(14, (uintptr_t)isr14, 0x08, INTERRUPT_GATE);
    
    // تعيين معالجات المقاطعات الخارجية (IRQ)
    set_idt_entry(32, (uintptr_t)irq0, 0x08, INTERRUPT_GATE);  // Timer
    set_idt_entry(33, (uintptr_t)irq1, 0x08, INTERRUPT_GATE);  // Keyboard
    set_idt_entry(34, (uintptr_t)irq2, 0x08, INTERRUPT_GATE);  // Cascade
    set_idt_entry(35, (uintptr_t)irq3, 0x08, INTERRUPT_GATE);  // COM2
    set_idt_entry(36, (uintptr_t)irq4, 0x08, INTERRUPT_GATE);  // COM1
    set_idt_entry(37, (uintptr_t)irq5, 0x08, INTERRUPT_GATE);  // LPT2
    set_idt_entry(38, (uintptr_t)irq6, 0x08, INTERRUPT_GATE);  // Floppy
    set_idt_entry(39, (uintptr_t)irq7, 0x08, INTERRUPT_GATE);  // LPT1
}

// دالة تهيئة PIC (Programmable Interrupt Controller)
void init_pic() {
    // إعادة تعيين PIC
    outb(0x20, 0x11);  // بدء تهيئة PIC الرئيسي
    outb(0xA0, 0x11);  // بدء تهيئة PIC الثانوي
    
    // تعيين أرقام المقاطعات
    outb(0x21, 0x20);  // PIC الرئيسي يبدأ من IRQ 32
    outb(0xA1, 0x28);  // PIC الثانوي يبدأ من IRQ 40
    
    // إعداد التسلسل
    outb(0x21, 0x04);  // إخبار PIC الرئيسي أن PIC الثانوي في IRQ2
    outb(0xA1, 0x02);  // إخبار PIC الثانوي برقم التسلسل
    
    // تعيين وضع 8086
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    
    // تفعيل جميع المقاطعات
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

// دالة تحميل IDT - معرفة في interrupt_asm.s
// void load_idt() - تم نقلها إلى assembly

// دالة تعيين مدخل في IDT
void set_idt_entry(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

// دالة تسجيل معالج مقاطعة
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler) {
    interrupt_handlers[num] = handler;
}

// دالة تفعيل المقاطعات
void enable_interrupts() {
    asm volatile("sti");
}

// دالة تعطيل المقاطعات
void disable_interrupts() {
    asm volatile("cli");
}

// معالج الاستثناءات العام
void isr_handler(interrupt_context_t* context) {
    interrupt_count++;
    
    if (interrupt_handlers[context->int_no] != NULL) {
        interrupt_handlers[context->int_no](context);
    } else {
        print_string("[KERNEL] استثناء غير معالج: ");
        print_hex(context->int_no);
        print_string("\n");
        print_interrupt_info(context);
    }
}

// معالج المقاطعات الخارجية العام
void irq_handler(interrupt_context_t* context) {
    interrupt_count++;
    
    // إرسال EOI
    send_eoi(context->int_no - 32);
    
    if (interrupt_handlers[context->int_no] != NULL) {
        interrupt_handlers[context->int_no](context);
    }
}

// معالج مقاطعة المؤقت
void timer_handler(interrupt_context_t* context) {
    timer_ticks++;
    
    // طباعة نقطة كل 100 tick
    if (timer_ticks % 100 == 0) {
        print_char('.');
    }
    
    // جدولة المهام كل 10 ticks
    if (timer_ticks % 10 == 0) {
        schedule(); // استدعاء جدولة المهام
    }
}

// معالج مقاطعة لوحة المفاتيح
void keyboard_handler(interrupt_context_t* context) {
    uint8_t scancode = inb(0x60);
    
    // طباعة رمز المفتاح (تنفيذ بسيط)
    print_string("[KB] مفتاح: ");
    print_hex(scancode);
    print_string("\n");
}

// معالج خطأ القسمة على صفر
void divide_error_handler(interrupt_context_t* context) {
    print_string("[ERROR] خطأ: القسمة على صفر!\n");
    print_interrupt_info(context);
    while(1); // توقف النظام
}

// معالج خطأ الصفحة
void page_fault_handler(interrupt_context_t* context) {
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    print_string("[ERROR] خطأ في الصفحة! العنوان: ");
    print_hex(faulting_address);
    print_string("\n");
    print_interrupt_info(context);
    while(1); // توقف النظام
}

// معالج خطأ الحماية العامة
void general_protection_fault_handler(interrupt_context_t* context) {
    print_string("[ERROR] خطأ في الحماية العامة!\n");
    print_interrupt_info(context);
    while(1); // توقف النظام
}

// دالة إرسال End of Interrupt
void send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20); // إرسال EOI إلى PIC الثانوي
    }
    outb(0x20, 0x20); // إرسال EOI إلى PIC الرئيسي
}

// دالة طباعة معلومات المقاطعة
void print_interrupt_info(interrupt_context_t* context) {
    print_string("رقم المقاطعة: ");
    print_hex(context->int_no);
    print_string(", رمز الخطأ: ");
    print_hex(context->err_code);
    print_string("\n");
    print_string("EIP: ");
    print_hex(context->eip);
    print_string(", CS: ");
    print_hex(context->cs);
    print_string("\n");
}

// دوال I/O البسيطة
void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

// دالة طباعة رقم بالنظام السادس عشر
void print_hex(uint32_t value) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    buffer[8] = '\0';
    
    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    print_string("0x");
    print_string(buffer);
}

// دالة الحصول على عدد المقاطعات
uint32_t get_interrupt_count() {
    return interrupt_count;
}

// دالة الحصول على عدد ticks المؤقت
uint32_t get_timer_ticks() {
    return timer_ticks;
}