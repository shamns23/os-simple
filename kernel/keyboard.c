#include "keyboard.h"
#include "kernel.h"
#include "interrupt.h"

// Forward declarations - التصريحات المسبقة
static void handle_key_press(uint8_t scancode);
static void handle_key_release(uint8_t scancode);
static bool is_special_key(uint8_t scancode);
static void handle_special_key(uint8_t scancode);

// Global keyboard state - الحالة العامة للوحة المفاتيح
static keyboard_state_t keyboard_state = {0};
static keyboard_buffer_t keyboard_buffer = {0};
static keyboard_stats_t keyboard_stats = {0};

// Scancode to ASCII translation table - جدول تحويل رمز المسح إلى ASCII
// مستوحى من Linux kernel 0.01 keyboard.c
static const char scancode_to_ascii_table[128] = {
    0,    27,   '1',  '2',  '3',  '4',  '5',  '6',   // 0x00-0x07
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',  // 0x08-0x0F
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',   // 0x10-0x17
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',   // 0x18-0x1F
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',   // 0x20-0x27
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',   // 0x28-0x2F
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',   // 0x30-0x37
    0,    ' ',  0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,    0,    0,    0,    0,    0,    '7',   // 0x40-0x47
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3',  '0',  '.',  0,    0,    0,    0,     // 0x50-0x57
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x58-0x5F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x60-0x67
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x68-0x6F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x70-0x77
    0,    0,    0,    0,    0,    0,    0,    0      // 0x78-0x7F
};

// Shifted characters table - جدول الأحرف مع Shift
static const char shifted_chars[128] = {
    0,    27,   '!',  '@',  '#',  '$',  '%',  '^',   // 0x00-0x07
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',  // 0x08-0x0F
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',   // 0x10-0x17
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',   // 0x18-0x1F
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',   // 0x20-0x27
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',   // 0x28-0x2F
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',   // 0x30-0x37
    0,    ' ',  0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,    0,    0,    0,    0,    0,    '7',   // 0x40-0x47
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3',  '0',  '.',  0,    0,    0,    0,     // 0x50-0x57
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x58-0x5F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x60-0x67
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x68-0x6F
    0,    0,    0,    0,    0,    0,    0,    0,     // 0x70-0x77
    0,    0,    0,    0,    0,    0,    0,    0      // 0x78-0x7F
};

/**
 * Initialize keyboard controller
 * تهيئة تحكم لوحة المفاتيح - مستوحى من Linux kernel 0.01
 */
void init_keyboard(void) {
    // Clear keyboard state
    keyboard_state.shift_pressed = false;
    keyboard_state.ctrl_pressed = false;
    keyboard_state.alt_pressed = false;
    keyboard_state.caps_lock = false;
    keyboard_state.num_lock = false;
    keyboard_state.scroll_lock = false;
    
    // Clear buffer
    keyboard_buffer.head = 0;
    keyboard_buffer.tail = 0;
    keyboard_buffer.count = 0;
    
    // Clear statistics
    keyboard_stats.total_keypresses = 0;
    keyboard_stats.total_releases = 0;
    keyboard_stats.special_keys = 0;
    keyboard_stats.buffer_overflows = 0;
    keyboard_stats.invalid_scancodes = 0;
    
    // Register keyboard interrupt handler (IRQ1)
    register_interrupt_handler(33, keyboard_interrupt_handler);
    
    print_string("[KEYBOARD] Keyboard initialized\n");
}

/**
 * Keyboard interrupt handler (IRQ1)
 * معالج مقاطعة لوحة المفاتيح - مستوحى من Linux kernel 0.01
 */
void keyboard_interrupt_handler(interrupt_context_t* context) {
    uint8_t scancode = read_keyboard_data();
    
    // Check if key was released
    bool key_released = (scancode & KEY_RELEASED) != 0;
    scancode &= 0x7F; // Remove release bit
    
    if (key_released) {
        keyboard_stats.total_releases++;
        handle_key_release(scancode);
    } else {
        keyboard_stats.total_keypresses++;
        handle_key_press(scancode);
    }
    
    // Send EOI to PIC
    outb(0x20, 0x20);
}

/**
 * Handle key press event
 * معالجة حدث ضغط المفتاح
 */
static void handle_key_press(uint8_t scancode) {
    // Handle special keys first
    if (is_special_key(scancode)) {
        handle_special_key(scancode);
        keyboard_stats.special_keys++;
        return;
    }
    
    // Convert scancode to ASCII
    char ascii = scancode_to_ascii(scancode);
    if (ascii != 0) {
        // Add to buffer if valid character
        if (!keyboard_buffer_put(ascii)) {
            keyboard_stats.buffer_overflows++;
        }
    } else {
        keyboard_stats.invalid_scancodes++;
    }
}

/**
 * Handle key release event
 * معالجة حدث تحرير المفتاح
 */
static void handle_key_release(uint8_t scancode) {
    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            keyboard_state.shift_pressed = false;
            break;
        case KEY_CTRL:
            keyboard_state.ctrl_pressed = false;
            break;
        case KEY_ALT:
            keyboard_state.alt_pressed = false;
            break;
    }
}

/**
 * Check if scancode represents a special key
 * فحص إذا كان رمز المسح يمثل مفتاح خاص
 */
static bool is_special_key(uint8_t scancode) {
    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
        case KEY_CTRL:
        case KEY_ALT:
        case KEY_CAPS:
            return true;
        default:
            return false;
    }
}

/**
 * Handle special key press
 * معالجة ضغط المفاتيح الخاصة
 */
static void handle_special_key(uint8_t scancode) {
    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            keyboard_state.shift_pressed = true;
            break;
        case KEY_CTRL:
            keyboard_state.ctrl_pressed = true;
            break;
        case KEY_ALT:
            keyboard_state.alt_pressed = true;
            break;
        case KEY_CAPS:
            keyboard_state.caps_lock = !keyboard_state.caps_lock;
            break;
    }
}

/**
 * Convert scancode to ASCII character
 * تحويل رمز المسح إلى حرف ASCII
 */
char scancode_to_ascii(uint8_t scancode) {
    if (scancode >= 128) {
        return 0; // Invalid scancode
    }
    
    char ascii = scancode_to_ascii_table[scancode];
    
    // Apply shift or caps lock
    if (keyboard_state.shift_pressed || 
        (keyboard_state.caps_lock && ascii >= 'a' && ascii <= 'z')) {
        if (scancode < 128) {
            char shifted = shifted_chars[scancode];
            if (shifted != 0) {
                ascii = shifted;
            }
        }
    }
    
    return ascii;
}

/**
 * Read character from keyboard buffer
 * قراءة حرف من مخزن لوحة المفاتيح
 */
char keyboard_getchar(void) {
    while (keyboard_buffer_is_empty()) {
        // Wait for input - في نظام حقيقي نستخدم halt أو yield
        asm volatile("hlt");
    }
    return keyboard_buffer_get();
}

/**
 * Check if keyboard has input available
 * فحص وجود إدخال متاح من لوحة المفاتيح
 */
bool keyboard_has_input(void) {
    return !keyboard_buffer_is_empty();
}

/**
 * Flush keyboard buffer
 * إفراغ مخزن لوحة المفاتيح
 */
void keyboard_flush_buffer(void) {
    keyboard_buffer.head = 0;
    keyboard_buffer.tail = 0;
    keyboard_buffer.count = 0;
}

/**
 * Add character to keyboard buffer
 * إضافة حرف إلى مخزن لوحة المفاتيح
 */
bool keyboard_buffer_put(char c) {
    if (keyboard_buffer_is_full()) {
        return false;
    }
    
    keyboard_buffer.buffer[keyboard_buffer.head] = c;
    keyboard_buffer.head = (keyboard_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
    keyboard_buffer.count++;
    
    return true;
}

/**
 * Get character from keyboard buffer
 * استخراج حرف من مخزن لوحة المفاتيح
 */
char keyboard_buffer_get(void) {
    if (keyboard_buffer_is_empty()) {
        return 0;
    }
    
    char c = keyboard_buffer.buffer[keyboard_buffer.tail];
    keyboard_buffer.tail = (keyboard_buffer.tail + 1) % KEYBOARD_BUFFER_SIZE;
    keyboard_buffer.count--;
    
    return c;
}

/**
 * Check if keyboard buffer is empty
 * فحص إذا كان مخزن لوحة المفاتيح فارغ
 */
bool keyboard_buffer_is_empty(void) {
    return keyboard_buffer.count == 0;
}

/**
 * Check if keyboard buffer is full
 * فحص إذا كان مخزن لوحة المفاتيح ممتلئ
 */
bool keyboard_buffer_is_full(void) {
    return keyboard_buffer.count >= KEYBOARD_BUFFER_SIZE;
}

/**
 * Wait for keyboard to be ready
 * انتظار جاهزية لوحة المفاتيح
 */
void wait_for_keyboard(void) {
    while (read_keyboard_status() & KEYBOARD_STATUS_INPUT_FULL) {
        // Wait until keyboard is ready
    }
}

/**
 * Read keyboard status register
 * قراءة سجل حالة لوحة المفاتيح
 */
uint8_t read_keyboard_status(void) {
    return inb(KEYBOARD_STATUS_PORT);
}

/**
 * Read keyboard data register
 * قراءة سجل بيانات لوحة المفاتيح
 */
uint8_t read_keyboard_data(void) {
    return inb(KEYBOARD_DATA_PORT);
}

/**
 * Send command to keyboard controller
 * إرسال أمر إلى تحكم لوحة المفاتيح
 */
void send_keyboard_command(uint8_t cmd) {
    wait_for_keyboard();
    outb(KEYBOARD_COMMAND_PORT, cmd);
}

/**
 * Print keyboard statistics
 * طباعة إحصائيات لوحة المفاتيح
 */
void print_keyboard_stats(void) {
    print_string("\n=== Keyboard Statistics ===\n");
    print_string("Total keypresses: ");
    print_number(keyboard_stats.total_keypresses);
    print_string("\nTotal releases: ");
    print_number(keyboard_stats.total_releases);
    print_string("\nSpecial keys: ");
    print_number(keyboard_stats.special_keys);
    print_string("\nBuffer overflows: ");
    print_number(keyboard_stats.buffer_overflows);
    print_string("\nInvalid scancodes: ");
    print_number(keyboard_stats.invalid_scancodes);
    print_string("\nBuffer count: ");
    print_number(keyboard_buffer.count);
    print_string("\n\nKeyboard state:\n");
    print_string("Shift: ");
    print_string(keyboard_state.shift_pressed ? "ON" : "OFF");
    print_string(" | Ctrl: ");
    print_string(keyboard_state.ctrl_pressed ? "ON" : "OFF");
    print_string(" | Alt: ");
    print_string(keyboard_state.alt_pressed ? "ON" : "OFF");
    print_string("\nCaps: ");
    print_string(keyboard_state.caps_lock ? "ON" : "OFF");
    print_string("\n");
}