#ifndef MEMORY_H
#define MEMORY_H

#include "kernel.h"

// ثوابت إدارة الذاكرة مستوحاة من Linux 0.01
#define PAGE_SIZE 4096              // حجم الصفحة 4KB
#define MEMORY_START 0x100000       // بداية الذاكرة المتاحة (1MB)
#define MEMORY_END 0x1000000        // نهاية الذاكرة (16MB)
#define MAX_PAGES ((MEMORY_END - MEMORY_START) / PAGE_SIZE)

// حالات الصفحات
#define PAGE_FREE 0
#define PAGE_USED 1
#define PAGE_RESERVED 2

// هيكل بيانات الصفحة
typedef struct {
    uint32_t address;           // عنوان الصفحة
    uint8_t status;             // حالة الصفحة (حرة/مستخدمة/محجوزة)
    uint32_t ref_count;         // عداد المراجع
} page_t;

// هيكل بيانات كتلة الذاكرة
typedef struct memory_block {
    uint32_t address;           // عنوان الكتلة
    uint32_t size;              // حجم الكتلة
    uint8_t is_free;            // هل الكتلة حرة؟
    struct memory_block* next;  // المؤشر للكتلة التالية
    struct memory_block* prev;  // المؤشر للكتلة السابقة
} memory_block_t;

// هيكل بيانات مدير الذاكرة
typedef struct {
    page_t pages[MAX_PAGES];    // مصفوفة الصفحات
    uint32_t total_pages;       // العدد الكلي للصفحات
    uint32_t free_pages;        // عدد الصفحات الحرة
    uint32_t used_pages;        // عدد الصفحات المستخدمة
    memory_block_t* free_list;  // قائمة الكتل الحرة
    uint32_t total_memory;      // إجمالي الذاكرة
    uint32_t free_memory;       // الذاكرة الحرة
} memory_manager_t;

// إحصائيات الذاكرة
typedef struct {
    uint32_t total_allocations; // إجمالي التخصيصات
    uint32_t total_frees;       // إجمالي التحريرات
    uint32_t current_allocated; // المخصص حالياً
    uint32_t peak_allocated;    // أقصى مخصص
    uint32_t fragmentation;     // نسبة التجزئة
} memory_stats_t;

// دوال إدارة الذاكرة الأساسية
void init_memory_manager(void);
void* kmalloc(uint32_t size);
void kfree(void* ptr);
void* kcalloc(uint32_t count, uint32_t size);
void* krealloc(void* ptr, uint32_t new_size);

// دوال إدارة الصفحات
void* alloc_page(void);
void free_page(void* page_addr);
page_t* get_page_info(void* addr);
uint32_t get_free_pages_count(void);

// دوال المراقبة والإحصائيات
void print_memory_info(void);
void print_memory_map(void);
memory_stats_t* get_memory_stats(void);
void check_memory_integrity(void);

// دوال مساعدة
void* memset(void* ptr, int value, uint32_t size);
void* memcpy(void* dest, const void* src, uint32_t size);
int memcmp(const void* ptr1, const void* ptr2, uint32_t size);
uint32_t align_address(uint32_t addr, uint32_t alignment);

// دوال التجزئة والضغط
void defragment_memory(void);
void compact_free_blocks(void);
uint32_t calculate_fragmentation(void);

// دوال الحماية والتحقق
int is_valid_address(void* addr);
int is_kernel_address(void* addr);
void mark_memory_region(uint32_t start, uint32_t end, uint8_t status);

#endif // MEMORY_H