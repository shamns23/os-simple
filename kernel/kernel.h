#ifndef KERNEL_H
#define KERNEL_H

// تعريفات الأنواع الأساسية
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// ثوابت VGA
#define VGA_TEXT_BUFFER 0xB8000
#define VGA_COLOR_WHITE 0x0F
#define VGA_COLOR_BLACK 0x00
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// تصريحات الدوال
void clear_screen();
void print_char(char c);
void print_string(const char* str);
void kernel_main();

#endif