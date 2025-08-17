#include "kernel.h"

// مؤشر إلى ذاكرة VGA
static uint16_t* vga_buffer = (uint16_t*)VGA_TEXT_BUFFER;
static int cursor_x = 0;
static int cursor_y = 0;

// دالة مسح الشاشة
void clear_screen() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (VGA_COLOR_BLACK << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

// دالة طباعة حرف واحد
void print_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        cursor_y = 0;
    }
    
    int index = cursor_y * VGA_WIDTH + cursor_x;
    vga_buffer[index] = (VGA_COLOR_WHITE << 8) | c;
    cursor_x++;
}

// دالة طباعة نص
void print_string(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

// الدالة الرئيسية للنواة
void kernel_main() {
    // مسح الشاشة
    clear_screen();
    
    // طباعة "hi"
    print_string("hi");
    
    // حلقة لا نهائية لمنع النواة من الانتهاء
    while (1) {
        // توقف المعالج
        __asm__("hlt");
    }
}