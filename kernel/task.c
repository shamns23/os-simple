#include "task.h"

// متغيرات عامة لإدارة المهام
task_t* current_task = 0;           // المهمة الحالية
task_t* task_list = 0;              // قائمة المهام
int next_pid = 1;                   // معرف المهمة التالي

// مصفوفة المهام - مبسطة
static task_t tasks[MAX_TASKS];
static int task_count = 0;

// تهيئة مدير المهام
void init_task_manager() {
    // مسح جميع المهام
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].pid = INVALID_PID;
        tasks[i].state = TASK_ZOMBIE;
        tasks[i].next = 0;
    }
    
    // إنشاء المهمة الأولى (kernel task)
    task_t* kernel_task = &tasks[0];
    kernel_task->pid = 0;
    kernel_task->state = TASK_RUNNING;
    kernel_task->priority = 0;
    kernel_task->parent_pid = INVALID_PID;
    
    // نسخ اسم المهمة
    const char* kernel_name = "kernel";
    for (int i = 0; i < 15 && kernel_name[i]; i++) {
        kernel_task->name[i] = kernel_name[i];
    }
    kernel_task->name[15] = '\0';
    
    current_task = kernel_task;
    task_list = kernel_task;
    task_count = 1;
    
    print_string("[TASK] Task manager initialized\n");
}

// إنشاء مهمة جديدة
task_t* create_task(const char* name, void* entry_point) {
    if (task_count >= MAX_TASKS) {
        print_string("[ERROR] Maximum tasks reached\n");
        return 0;
    }
    
    // البحث عن مكان فارغ
    task_t* new_task = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].pid == INVALID_PID) {
            new_task = &tasks[i];
            break;
        }
    }
    
    if (!new_task) {
        print_string("[ERROR] No free task slot\n");
        return 0;
    }
    
    // تهيئة المهمة الجديدة
    new_task->pid = next_pid++;
    new_task->state = TASK_READY;
    new_task->priority = 10;  // أولوية افتراضية
    new_task->parent_pid = current_task ? current_task->pid : INVALID_PID;
    new_task->eip = (uint32_t)entry_point;
    
    // نسخ اسم المهمة
    for (int i = 0; i < 15 && name[i]; i++) {
        new_task->name[i] = name[i];
    }
    new_task->name[15] = '\0';
    
    // إضافة المهمة إلى القائمة
    if (task_list) {
        task_t* last = task_list;
        while (last->next) {
            last = last->next;
        }
        last->next = new_task;
    } else {
        task_list = new_task;
    }
    
    task_count++;
    
    print_string("[TASK] Created task: ");
    print_string(name);
    print_string("\n");
    
    return new_task;
}

// جدولة المهام البسيطة - Round Robin
void schedule() {
    if (!current_task || !current_task->next) {
        return;
    }
    
    // البحث عن المهمة التالية الجاهزة
    task_t* next_task = current_task->next;
    while (next_task && next_task->state != TASK_READY && next_task->state != TASK_RUNNING) {
        next_task = next_task->next;
        if (!next_task) {
            next_task = task_list;  // العودة إلى بداية القائمة
        }
        if (next_task == current_task) {
            break;  // تجنب الحلقة اللا نهائية
        }
    }
    
    if (next_task && next_task != current_task) {
        task_t* prev_task = current_task;
        current_task = next_task;
        current_task->state = TASK_RUNNING;
        
        if (prev_task->state == TASK_RUNNING) {
            prev_task->state = TASK_READY;
        }
        
        // هنا يمكن إضافة تبديل السياق الفعلي
        // switch_to_task(current_task);
    }
}

// إنهاء المهمة
void task_exit(int exit_code) {
    if (current_task) {
        current_task->state = TASK_ZOMBIE;
        print_string("[TASK] Task ");
        print_string(current_task->name);
        print_string(" exited\n");
        
        // جدولة المهمة التالية
        schedule();
    }
}

// إيقاف المهمة مؤقتاً
void task_sleep(int ticks) {
    if (current_task) {
        current_task->state = TASK_SLEEPING;
        schedule();
    }
}

// إيقاظ المهمة
void task_wake(task_t* task) {
    if (task && task->state == TASK_SLEEPING) {
        task->state = TASK_READY;
    }
}

// البحث عن مهمة بالمعرف
task_t* find_task(int pid) {
    task_t* task = task_list;
    while (task) {
        if (task->pid == pid) {
            return task;
        }
        task = task->next;
    }
    return 0;
}

// طباعة معلومات المهام
void print_task_info() {
    print_string("\n=== Task Information ===\n");
    print_string("Current task: ");
    if (current_task) {
        print_string(current_task->name);
    } else {
        print_string("None");
    }
    print_string("\n");
    
    print_string("Task list:\n");
    task_t* task = task_list;
    while (task) {
        print_string("  PID: ");
        // طباعة رقم PID (مبسطة)
        if (task->pid < 10) {
            print_char('0' + task->pid);
        } else {
            print_char('0' + (task->pid / 10));
            print_char('0' + (task->pid % 10));
        }
        
        print_string(" Name: ");
        print_string(task->name);
        
        print_string(" State: ");
        switch (task->state) {
            case TASK_RUNNING:
                print_string("RUNNING");
                break;
            case TASK_READY:
                print_string("READY");
                break;
            case TASK_SLEEPING:
                print_string("SLEEPING");
                break;
            case TASK_ZOMBIE:
                print_string("ZOMBIE");
                break;
            default:
                print_string("UNKNOWN");
        }
        print_string("\n");
        
        task = task->next;
    }
    print_string("========================\n");
}

// طباعة معلومات مهمة محددة بالمعرف
void print_task_info_by_pid(int pid) {
    task_t* task = find_task(pid);
    if (!task) {
        print_string("[ERROR] Task with PID ");
        print_number(pid);
        print_string(" not found\n");
        return;
    }
    
    print_string("Task PID: ");
    print_number(task->pid);
    print_string(" Name: ");
    print_string(task->name);
    print_string(" State: ");
    
    switch (task->state) {
        case TASK_RUNNING:
            print_string("RUNNING");
            break;
        case TASK_READY:
            print_string("READY");
            break;
        case TASK_SLEEPING:
            print_string("SLEEPING");
            break;
        case TASK_ZOMBIE:
            print_string("ZOMBIE");
            break;
        default:
            print_string("UNKNOWN");
    }
    print_string("\n");
}

// حفظ سياق المهمة (مبسط)
void save_task_context(task_t* task) {
    // في تطبيق حقيقي، هنا نحفظ جميع السجلات
    // __asm__ volatile ("mov %%esp, %0" : "=m" (task->esp));
    // __asm__ volatile ("mov %%ebp, %0" : "=m" (task->ebp));
}

// استعادة سياق المهمة (مبسط)
void restore_task_context(task_t* task) {
    // في تطبيق حقيقي، هنا نستعيد جميع السجلات
    // __asm__ volatile ("mov %0, %%esp" : : "m" (task->esp));
    // __asm__ volatile ("mov %0, %%ebp" : : "m" (task->ebp));
}

// التبديل إلى مهمة (مبسط)
void switch_to_task(task_t* task) {
    if (current_task) {
        save_task_context(current_task);
    }
    
    current_task = task;
    restore_task_context(task);
}