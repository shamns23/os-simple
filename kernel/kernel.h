#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stdbool.h>

// تعريفات إضافية للنظام
#define NULL ((void*)0)
#define TRUE 1
#define FALSE 0

// دوال الإدخال والإخراج
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void print_hex(uint32_t value);
uint32_t get_interrupt_count(void);
uint32_t get_timer_ticks(void);

// دوال إدارة الذاكرة
void init_memory_manager(void);
void* kmalloc(uint32_t size);
void kfree(void* ptr);
void print_memory_info(void);

// دوال استدعاءات النظام
void init_syscalls(void);
void print_syscall_stats(void);
void test_syscalls(void);
void syscall_print(const char* msg);

// Scheduler functions
void init_scheduler(void);
void start_scheduler(void);
void schedule(void);
void yield(void);
void print_scheduler_stats(void);

// Keyboard functions
void init_keyboard(void);
char keyboard_getchar(void);
bool keyboard_has_input(void);
void keyboard_flush_buffer(void);
void print_keyboard_stats(void);

// ثوابت VGA
#define VGA_TEXT_BUFFER 0xB8000
#define VGA_COLOR_WHITE 0x0F
#define VGA_COLOR_BLACK 0x00
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// تصريحات الدوال
void clear_screen();
void print_char(char c);
void print(const char* str);
void print_string(const char* str);
void print_number(uint32_t num);
void scroll_screen(void);
void kernel_main();

#endif