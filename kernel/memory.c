#include "memory.h"
#include "kernel.h"

// متغيرات عامة لإدارة الذاكرة
static memory_manager_t memory_manager;
static memory_stats_t memory_stats;
static uint8_t memory_initialized = 0;

// دالة تهيئة مدير الذاكرة
void init_memory_manager(void) {
    uint32_t i;
    uint32_t page_addr;
    
    // تهيئة مدير الذاكرة
    memory_manager.total_pages = MAX_PAGES;
    memory_manager.free_pages = MAX_PAGES;
    memory_manager.used_pages = 0;
    memory_manager.free_list = NULL;
    memory_manager.total_memory = MEMORY_END - MEMORY_START;
    memory_manager.free_memory = memory_manager.total_memory;
    
    // تهيئة مصفوفة الصفحات
    page_addr = MEMORY_START;
    for (i = 0; i < MAX_PAGES; i++) {
        memory_manager.pages[i].address = page_addr;
        memory_manager.pages[i].status = PAGE_FREE;
        memory_manager.pages[i].ref_count = 0;
        page_addr += PAGE_SIZE;
    }
    
    // تهيئة الإحصائيات
    memory_stats.total_allocations = 0;
    memory_stats.total_frees = 0;
    memory_stats.current_allocated = 0;
    memory_stats.peak_allocated = 0;
    memory_stats.fragmentation = 0;
    
    // إنشاء كتلة حرة كبيرة في البداية
    memory_block_t* initial_block = (memory_block_t*)MEMORY_START;
    initial_block->address = MEMORY_START + sizeof(memory_block_t);
    initial_block->size = memory_manager.total_memory - sizeof(memory_block_t);
    initial_block->is_free = 1;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    memory_manager.free_list = initial_block;
    
    memory_initialized = 1;
    
    print_string("Memory Manager: تم تهيئة مدير الذاكرة\n");
    print_string("Total Memory: ");
    print_hex(memory_manager.total_memory);
    print_string(" bytes\n");
}

// دالة تخصيص الذاكرة (مشابهة لـ malloc)
void* kmalloc(uint32_t size) {
    memory_block_t* current;
    memory_block_t* new_block;
    uint32_t aligned_size;
    
    if (!memory_initialized || size == 0) {
        return NULL;
    }
    
    // محاذاة الحجم إلى 4 بايت
    aligned_size = align_address(size, 4);
    
    // البحث عن كتلة حرة مناسبة
    current = memory_manager.free_list;
    while (current != NULL) {
        if (current->is_free && current->size >= aligned_size) {
            // إذا كانت الكتلة أكبر من المطلوب، قسمها
            if (current->size > aligned_size + sizeof(memory_block_t)) {
                new_block = (memory_block_t*)(current->address + aligned_size);
                new_block->address = current->address + aligned_size + sizeof(memory_block_t);
                new_block->size = current->size - aligned_size - sizeof(memory_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                new_block->prev = current;
                
                if (current->next) {
                    current->next->prev = new_block;
                }
                current->next = new_block;
                current->size = aligned_size;
            }
            
            current->is_free = 0;
            
            // تحديث الإحصائيات
            memory_stats.total_allocations++;
            memory_stats.current_allocated += current->size;
            if (memory_stats.current_allocated > memory_stats.peak_allocated) {
                memory_stats.peak_allocated = memory_stats.current_allocated;
            }
            memory_manager.free_memory -= current->size;
            
            return (void*)current->address;
        }
        current = current->next;
    }
    
    return NULL; // لا توجد ذاكرة كافية
}

// دالة تحرير الذاكرة (مشابهة لـ free)
void kfree(void* ptr) {
    memory_block_t* current;
    memory_block_t* block_to_free = NULL;
    
    if (!memory_initialized || ptr == NULL) {
        return;
    }
    
    // البحث عن الكتلة المراد تحريرها
    current = memory_manager.free_list;
    while (current != NULL) {
        if (current->address == (uint32_t)ptr) {
            block_to_free = current;
            break;
        }
        current = current->next;
    }
    
    if (block_to_free == NULL || block_to_free->is_free) {
        return; // الكتلة غير موجودة أو محررة مسبقاً
    }
    
    // تحرير الكتلة
    block_to_free->is_free = 1;
    
    // تحديث الإحصائيات
    memory_stats.total_frees++;
    memory_stats.current_allocated -= block_to_free->size;
    memory_manager.free_memory += block_to_free->size;
    
    // دمج الكتل المجاورة الحرة
    compact_free_blocks();
}

// دالة تخصيص ذاكرة مع التصفير (مشابهة لـ calloc)
void* kcalloc(uint32_t count, uint32_t size) {
    uint32_t total_size = count * size;
    void* ptr = kmalloc(total_size);
    
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

// دالة إعادة تخصيص الذاكرة (مشابهة لـ realloc)
void* krealloc(void* ptr, uint32_t new_size) {
    void* new_ptr;
    memory_block_t* current;
    uint32_t old_size = 0;
    
    if (ptr == NULL) {
        return kmalloc(new_size);
    }
    
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // البحث عن حجم الكتلة الحالية
    current = memory_manager.free_list;
    while (current != NULL) {
        if (current->address == (uint32_t)ptr) {
            old_size = current->size;
            break;
        }
        current = current->next;
    }
    
    new_ptr = kmalloc(new_size);
    if (new_ptr != NULL && old_size > 0) {
        uint32_t copy_size = (old_size < new_size) ? old_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
        kfree(ptr);
    }
    
    return new_ptr;
}

// دالة تخصيص صفحة
void* alloc_page(void) {
    uint32_t i;
    
    for (i = 0; i < MAX_PAGES; i++) {
        if (memory_manager.pages[i].status == PAGE_FREE) {
            memory_manager.pages[i].status = PAGE_USED;
            memory_manager.pages[i].ref_count = 1;
            memory_manager.free_pages--;
            memory_manager.used_pages++;
            return (void*)memory_manager.pages[i].address;
        }
    }
    
    return NULL; // لا توجد صفحات حرة
}

// دالة تحرير صفحة
void free_page(void* page_addr) {
    uint32_t i;
    uint32_t addr = (uint32_t)page_addr;
    
    for (i = 0; i < MAX_PAGES; i++) {
        if (memory_manager.pages[i].address == addr) {
            if (memory_manager.pages[i].status == PAGE_USED) {
                memory_manager.pages[i].ref_count--;
                if (memory_manager.pages[i].ref_count == 0) {
                    memory_manager.pages[i].status = PAGE_FREE;
                    memory_manager.free_pages++;
                    memory_manager.used_pages--;
                }
            }
            return;
        }
    }
}

// دالة الحصول على معلومات الصفحة
page_t* get_page_info(void* addr) {
    uint32_t i;
    uint32_t page_addr = (uint32_t)addr & ~(PAGE_SIZE - 1);
    
    for (i = 0; i < MAX_PAGES; i++) {
        if (memory_manager.pages[i].address == page_addr) {
            return &memory_manager.pages[i];
        }
    }
    
    return NULL;
}

// دالة الحصول على عدد الصفحات الحرة
uint32_t get_free_pages_count(void) {
    return memory_manager.free_pages;
}

// دالة طباعة معلومات الذاكرة
void print_memory_info(void) {
    print_string("\n=== Memory Information ===\n");
    print_string("Total Memory: ");
    print_hex(memory_manager.total_memory);
    print_string(" bytes\n");
    
    print_string("Free Memory: ");
    print_hex(memory_manager.free_memory);
    print_string(" bytes\n");
    
    print_string("Used Memory: ");
    print_hex(memory_manager.total_memory - memory_manager.free_memory);
    print_string(" bytes\n");
    
    print_string("Total Pages: ");
    print_hex(memory_manager.total_pages);
    print_string("\n");
    
    print_string("Free Pages: ");
    print_hex(memory_manager.free_pages);
    print_string("\n");
    
    print_string("Used Pages: ");
    print_hex(memory_manager.used_pages);
    print_string("\n");
    
    print_string("Allocations: ");
    print_hex(memory_stats.total_allocations);
    print_string("\n");
    
    print_string("Frees: ");
    print_hex(memory_stats.total_frees);
    print_string("\n");
}

// دالة الحصول على الإحصائيات
memory_stats_t* get_memory_stats(void) {
    return &memory_stats;
}

// دالة ضغط الكتل الحرة
void compact_free_blocks(void) {
    memory_block_t* current = memory_manager.free_list;
    
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            // دمج الكتلتين إذا كانتا متجاورتين
            if (current->address + current->size == current->next->address - sizeof(memory_block_t)) {
                current->size += current->next->size + sizeof(memory_block_t);
                memory_block_t* to_remove = current->next;
                current->next = to_remove->next;
                if (to_remove->next) {
                    to_remove->next->prev = current;
                }
                continue;
            }
        }
        current = current->next;
    }
}

// دالة محاذاة العنوان
uint32_t align_address(uint32_t addr, uint32_t alignment) {
    return (addr + alignment - 1) & ~(alignment - 1);
}

// دالة التحقق من صحة العنوان
int is_valid_address(void* addr) {
    uint32_t address = (uint32_t)addr;
    return (address >= MEMORY_START && address < MEMORY_END);
}

// دالة التحقق من عنوان النواة
int is_kernel_address(void* addr) {
    uint32_t address = (uint32_t)addr;
    return (address >= MEMORY_START && address < MEMORY_START + 0x100000); // أول 1MB للنواة
}

// دوال الذاكرة الأساسية
void* memset(void* ptr, int value, uint32_t size) {
    uint8_t* p = (uint8_t*)ptr;
    uint32_t i;
    
    for (i = 0; i < size; i++) {
        p[i] = (uint8_t)value;
    }
    
    return ptr;
}

void* memcpy(void* dest, const void* src, uint32_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    uint32_t i;
    
    for (i = 0; i < size; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, uint32_t size) {
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;
    uint32_t i;
    
    for (i = 0; i < size; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    
    return 0;
}