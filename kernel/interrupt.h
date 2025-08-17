#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "kernel.h"

// ثوابت المقاطعات - مستوحاة من Linux 0.01
#define IDT_SIZE 256                    // حجم جدول وصف المقاطعات
#define INTERRUPT_GATE 0x8E             // نوع بوابة المقاطعة
#define TRAP_GATE 0x8F                  // نوع بوابة الفخ

// أرقام المقاطعات الأساسية
#define IRQ_TIMER       0x20            // مقاطعة المؤقت
#define IRQ_KEYBOARD    0x21            // مقاطعة لوحة المفاتيح
#define IRQ_CASCADE     0x22            // مقاطعة التسلسل
#define IRQ_COM2        0x23            // مقاطعة COM2
#define IRQ_COM1        0x24            // مقاطعة COM1
#define IRQ_LPT2        0x25            // مقاطعة LPT2
#define IRQ_FLOPPY      0x26            // مقاطعة القرص المرن
#define IRQ_LPT1        0x27            // مقاطعة LPT1

// مقاطعات الاستثناءات
#define EXCEPTION_DIVIDE_ERROR          0x00
#define EXCEPTION_DEBUG                 0x01
#define EXCEPTION_NMI                   0x02
#define EXCEPTION_BREAKPOINT            0x03
#define EXCEPTION_OVERFLOW              0x04
#define EXCEPTION_BOUND_RANGE           0x05
#define EXCEPTION_INVALID_OPCODE        0x06
#define EXCEPTION_DEVICE_NOT_AVAILABLE  0x07
#define EXCEPTION_DOUBLE_FAULT          0x08
#define EXCEPTION_INVALID_TSS           0x0A
#define EXCEPTION_SEGMENT_NOT_PRESENT   0x0B
#define EXCEPTION_STACK_FAULT           0x0C
#define EXCEPTION_GENERAL_PROTECTION    0x0D
#define EXCEPTION_PAGE_FAULT            0x0E

// هيكل وصف المقاطعة في IDT
typedef struct {
    uint16_t offset_low;    // الجزء السفلي من عنوان معالج المقاطعة
    uint16_t selector;      // محدد الشريحة
    uint8_t zero;           // محجوز (يجب أن يكون صفر)
    uint8_t type_attr;      // نوع وخصائص المقاطعة
    uint16_t offset_high;   // الجزء العلوي من عنوان معالج المقاطعة
} __attribute__((packed)) idt_entry_t;

// مؤشر IDT
typedef struct {
    uint16_t limit;         // حجم IDT
    uint32_t base;          // عنوان IDT
} __attribute__((packed)) idt_ptr_t;

// هيكل سياق المقاطعة
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // السجلات العامة
    uint32_t int_no, err_code;                        // رقم المقاطعة ورمز الخطأ
    uint32_t eip, cs, eflags, useresp, ss;           // سجلات النظام
} interrupt_context_t;

// نوع دالة معالج المقاطعة
typedef void (*interrupt_handler_t)(interrupt_context_t* context);

// متغيرات عامة
extern idt_entry_t idt[IDT_SIZE];       // جدول وصف المقاطعات
extern idt_ptr_t idt_ptr;               // مؤشر IDT
extern interrupt_handler_t interrupt_handlers[IDT_SIZE]; // معالجات المقاطعات

// دوال تهيئة المقاطعات
void init_interrupts();                 // تهيئة نظام المقاطعات
void init_idt();                        // تهيئة IDT
void init_pic();                        // تهيئة PIC (Programmable Interrupt Controller)
void load_idt();                        // تحميل IDT

// دوال إدارة المقاطعات
void set_idt_entry(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler);
void enable_interrupts();               // تفعيل المقاطعات
void disable_interrupts();              // تعطيل المقاطعات

// معالجات المقاطعات الأساسية
void isr_handler(interrupt_context_t* context);    // معالج الاستثناءات
void irq_handler(interrupt_context_t* context);    // معالج المقاطعات الخارجية

// معالجات مقاطعات محددة
void timer_handler(interrupt_context_t* context);     // معالج مقاطعة المؤقت
void keyboard_handler(interrupt_context_t* context);  // معالج مقاطعة لوحة المفاتيح
void divide_error_handler(interrupt_context_t* context); // معالج خطأ القسمة على صفر
void page_fault_handler(interrupt_context_t* context); // معالج خطأ الصفحة
void general_protection_fault_handler(interrupt_context_t* context); // معالج خطأ الحماية العامة

// دوال مساعدة
void send_eoi(uint8_t irq);             // إرسال End of Interrupt
void print_interrupt_info(interrupt_context_t* context); // طباعة معلومات المقاطعة

// ماكرو لتعريف معالجات المقاطعات في Assembly
#define ISR_NOERRCODE(num) \
    extern void isr##num(); \
    void isr##num##_handler(interrupt_context_t* context);

#define ISR_ERRCODE(num) \
    extern void isr##num(); \
    void isr##num##_handler(interrupt_context_t* context);

#define IRQ(num, irq_num) \
    extern void irq##num(); \
    void irq##num##_handler(interrupt_context_t* context);

// تصريحات معالجات الاستثناءات
ISR_NOERRCODE(0)   // Divide by zero
ISR_NOERRCODE(1)   // Debug
ISR_NOERRCODE(2)   // Non-maskable interrupt
ISR_NOERRCODE(3)   // Breakpoint
ISR_NOERRCODE(4)   // Overflow
ISR_NOERRCODE(5)   // Bound range exceeded
ISR_NOERRCODE(6)   // Invalid opcode
ISR_NOERRCODE(7)   // Device not available
ISR_ERRCODE(8)     // Double fault
ISR_ERRCODE(10)    // Invalid TSS
ISR_ERRCODE(11)    // Segment not present
ISR_ERRCODE(12)    // Stack fault
ISR_ERRCODE(13)    // General protection fault
ISR_ERRCODE(14)    // Page fault

// تصريحات معالجات المقاطعات الخارجية
IRQ(0, 32)         // Timer
IRQ(1, 33)         // Keyboard
IRQ(2, 34)         // Cascade
IRQ(3, 35)         // COM2
IRQ(4, 36)         // COM1
IRQ(5, 37)         // LPT2
IRQ(6, 38)         // Floppy
IRQ(7, 39)         // LPT1

#endif