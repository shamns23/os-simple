#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "interrupt.h"

// Keyboard controller ports - منافذ تحكم لوحة المفاتيح
#define KEYBOARD_DATA_PORT    0x60  // منفذ البيانات
#define KEYBOARD_STATUS_PORT  0x64  // منفذ الحالة
#define KEYBOARD_COMMAND_PORT 0x64  // منفذ الأوامر

// Keyboard status register bits - بتات سجل حالة لوحة المفاتيح
#define KEYBOARD_STATUS_OUTPUT_FULL  0x01  // البيانات جاهزة للقراءة
#define KEYBOARD_STATUS_INPUT_FULL   0x02  // المعالج مشغول
#define KEYBOARD_STATUS_SYSTEM_FLAG  0x04  // علم النظام
#define KEYBOARD_STATUS_COMMAND_DATA 0x08  // نوع البيانات
#define KEYBOARD_STATUS_TIMEOUT      0x40  // انتهاء المهلة الزمنية
#define KEYBOARD_STATUS_PARITY_ERROR 0x80  // خطأ التكافؤ

// Special key codes - رموز المفاتيح الخاصة
#define KEY_ESC       0x01
#define KEY_BACKSPACE 0x0E
#define KEY_TAB       0x0F
#define KEY_ENTER     0x1C
#define KEY_CTRL      0x1D
#define KEY_LSHIFT    0x2A
#define KEY_RSHIFT    0x36
#define KEY_ALT       0x38
#define KEY_SPACE     0x39
#define KEY_CAPS      0x3A
#define KEY_F1        0x3B
#define KEY_F2        0x3C
#define KEY_F3        0x3D
#define KEY_F4        0x3E
#define KEY_F5        0x3F
#define KEY_F6        0x40
#define KEY_F7        0x41
#define KEY_F8        0x42
#define KEY_F9        0x43
#define KEY_F10       0x44
#define KEY_F11       0x57
#define KEY_F12       0x58

// Key states - حالات المفاتيح
#define KEY_PRESSED   0x00
#define KEY_RELEASED  0x80

// Keyboard buffer size - حجم مخزن لوحة المفاتيح
#define KEYBOARD_BUFFER_SIZE 256

// Keyboard state structure - هيكل حالة لوحة المفاتيح
typedef struct {
    bool shift_pressed;    // مفتاح Shift مضغوط
    bool ctrl_pressed;     // مفتاح Ctrl مضغوط
    bool alt_pressed;      // مفتاح Alt مضغوط
    bool caps_lock;        // Caps Lock مفعل
    bool num_lock;         // Num Lock مفعل
    bool scroll_lock;      // Scroll Lock مفعل
} keyboard_state_t;

// Keyboard buffer structure - هيكل مخزن لوحة المفاتيح
typedef struct {
    char buffer[KEYBOARD_BUFFER_SIZE];  // المخزن
    uint32_t head;                      // مؤشر الرأس
    uint32_t tail;                      // مؤشر الذيل
    uint32_t count;                     // عدد الأحرف
} keyboard_buffer_t;

// Keyboard statistics - إحصائيات لوحة المفاتيح
typedef struct {
    uint32_t total_keypresses;    // إجمالي ضغطات المفاتيح
    uint32_t total_releases;      // إجمالي تحرير المفاتيح
    uint32_t special_keys;        // المفاتيح الخاصة
    uint32_t buffer_overflows;    // تجاوز المخزن
    uint32_t invalid_scancodes;   // رموز المسح غير الصالحة
} keyboard_stats_t;

// Function declarations - إعلانات الدوال
void init_keyboard(void);                    // تهيئة لوحة المفاتيح
void keyboard_interrupt_handler(interrupt_context_t* context); // معالج مقاطعة لوحة المفاتيح
char keyboard_getchar(void);                 // قراءة حرف من المخزن
bool keyboard_has_input(void);               // فحص وجود إدخال
void keyboard_flush_buffer(void);            // إفراغ المخزن
char scancode_to_ascii(uint8_t scancode);    // تحويل رمز المسح إلى ASCII
// Internal functions are now static in keyboard.c
void print_keyboard_stats(void);             // طباعة الإحصائيات

// Buffer management - إدارة المخزن
bool keyboard_buffer_put(char c);            // إضافة حرف للمخزن
char keyboard_buffer_get(void);              // استخراج حرف من المخزن
bool keyboard_buffer_is_empty(void);         // فحص إذا كان المخزن فارغ
bool keyboard_buffer_is_full(void);          // فحص إذا كان المخزن ممتلئ

// Utility functions - دوال مساعدة
void wait_for_keyboard(void);                // انتظار جاهزية لوحة المفاتيح
uint8_t read_keyboard_status(void);          // قراءة حالة لوحة المفاتيح
uint8_t read_keyboard_data(void);            // قراءة بيانات لوحة المفاتيح
void send_keyboard_command(uint8_t cmd);     // إرسال أمر للوحة المفاتيح

#endif // KEYBOARD_H