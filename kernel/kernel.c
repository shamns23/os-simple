#include "kernel.h"
#include "task.h"
#include "interrupt.h"
#include "memory.h"
#include "syscall.h"
#include "scheduler.h"
#include "keyboard.h"

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
        if (cursor_y >= VGA_HEIGHT) {
            cursor_y = VGA_HEIGHT - 1;
            scroll_screen();
        }
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)c | (VGA_COLOR_WHITE << 8);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= VGA_HEIGHT) {
                cursor_y = VGA_HEIGHT - 1;
                scroll_screen();
            }
        }
    }
}

void scroll_screen(void) {
    // تمرير الشاشة لأعلى - نسخ كل سطر إلى السطر الذي فوقه
    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga_buffer[(row - 1) * VGA_WIDTH + col] = vga_buffer[row * VGA_WIDTH + col];
        }
    }
    
    // مسح السطر الأخير
    for (int col = 0; col < VGA_WIDTH; col++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = ' ' | (7 << 8); // 7 = light grey
    }
    
    // تحديث موضع المؤشر
    cursor_y = VGA_HEIGHT - 1;
    cursor_x = 0;
}

void print_number(uint32_t num) {
    char buffer[12]; // كافي لـ 32-bit number
    int i = 0;
    
    if (num == 0) {
        print_char('0');
        return;
    }
    
    // تحويل الرقم إلى نص
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // طباعة الأرقام بالترتيب الصحيح
    for (int j = i - 1; j >= 0; j--) {
        print_char(buffer[j]);
    }
}

// دالة طباعة نص
void print_string(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

// مهمة تجريبية بسيطة
void demo_task() {
    for (int i = 0; i < 3; i++) {
        print_string("[DEMO] Task running...\n");
        // محاكاة عمل
        for (volatile int j = 0; j < 1000000; j++);
    }
    task_exit(0);
}

// الدالة الرئيسية للنواة
void kernel_main() {
    // تنظيف الشاشة
    clear_screen();
    
    // طباعة رسالة الترحيب
    print_string("[KERNEL] بدء تشغيل النواة...\n");
    print_string("[KERNEL] مرحباً بك في نظام التشغيل البسيط!\n\n");
    
    // تهيئة نظام المقاطعات
    print_string("[KERNEL] تهيئة نظام المقاطعات...\n");
    init_interrupts();
    
    // تهيئة مدير الذاكرة
    init_memory_manager();
    
    // تهيئة نظام استدعاءات النظام
    init_syscalls();
    
    // Initialize scheduler
    init_scheduler();
    
    // Start scheduler
    start_scheduler();
    
    // تهيئة مدير المهام
    print_string("[KERNEL] تهيئة مدير المهام...\n");
    init_task_manager();
    
    // إنشاء مهمة تجريبية
    print_string("[KERNEL] إنشاء مهمة تجريبية...\n");
    create_task("demo_task", (void*)demo_task);
    
    // طباعة معلومات المهام
    print_string("\n[KERNEL] معلومات المهام الحالية:\n");
    print_task_info_by_pid(0); // المهمة الرئيسية
    print_task_info_by_pid(1); // المهمة التجريبية
    
    // تفعيل المقاطعات
    print_string("\n[KERNEL] تفعيل المقاطعات...\n");
    enable_interrupts();
    
    print_string("\n[KERNEL] النواة جاهزة!\n");
    print_string("hi\n");
    print_string("[INFO] النظام يعمل الآن مع دعم المقاطعات والمهام\n");
    print_string("[INFO] اضغط أي مفتاح لاختبار لوحة المفاتيح\n");
    
    // اختبار إدارة الذاكرة
    print_string("\n=== اختبار إدارة الذاكرة ===\n");
    
    // تخصيص بعض الذاكرة للاختبار
    void* ptr1 = kmalloc(1024);
    void* ptr2 = kmalloc(2048);
    void* ptr3 = kmalloc(512);
    
    print_string("تم تخصيص 3 كتل ذاكرة\n");
    print_memory_info();
    
    // تحرير بعض الذاكرة
    kfree(ptr2);
    print_string("\nتم تحرير الكتلة الثانية\n");
    print_memory_info();
    
    // Initialize keyboard
    init_keyboard();
    
    // Test keyboard interaction
    print_string("\n=== Keyboard Test ===\n");
    print_string("Type some characters (press any key to continue):\n");
    
    // Simple keyboard test - wait for a few keypresses
    for (int i = 0; i < 5; i++) {
        if (keyboard_has_input()) {
            char c = keyboard_getchar();
            print_string("You pressed: ");
            print_char(c);
            print_string("\n");
        } else {
            print_string("Waiting for input...\n");
            // In a real system, we would use proper scheduling here
            for (volatile int j = 0; j < 1000000; j++); // Simple delay
        }
    }
    
    // Display keyboard statistics
    print_keyboard_stats();
    
    print_string("\n=== System Ready ===\n");
    print_string("All Linux 0.01 inspired features initialized!\n");
    print_string("[DEBUG] Entering main loop...\n");
    
    // Keep system running
    while (1) {
        if (keyboard_has_input()) {
            char c = keyboard_getchar();
            print_string("Input: ");
            print_char(c);
            print_string("\n");
        }
        asm volatile("hlt"); // Halt until next interrupt
    }
}