#ifndef TASK_H
#define TASK_H

#include "kernel.h"

// حالات المهام - مستوحاة من Linux 0.01
#define TASK_RUNNING     0  // المهمة قيد التشغيل
#define TASK_READY       1  // المهمة جاهزة للتشغيل
#define TASK_SLEEPING    2  // المهمة نائمة
#define TASK_ZOMBIE      3  // المهمة منتهية ولكن لم يتم تنظيفها

// معرف المهمة
#define MAX_TASKS        64
#define INVALID_PID      -1

// هيكل بيانات المهمة - مبسط من Linux 0.01
typedef struct task_struct {
    int pid;                    // معرف العملية
    int state;                  // حالة المهمة
    int priority;               // أولوية المهمة
    uint32_t esp;              // مؤشر المكدس
    uint32_t ebp;              // مؤشر القاعدة
    uint32_t eip;              // مؤشر التعليمة
    uint32_t cr3;              // سجل صفحات الذاكرة
    
    // معلومات إضافية
    char name[16];             // اسم المهمة
    int parent_pid;            // معرف المهمة الأب
    uint32_t start_time;       // وقت بداية المهمة
    
    // مؤشر للمهمة التالية في القائمة
    struct task_struct* next;
} task_t;

// متغيرات عامة لإدارة المهام
extern task_t* current_task;        // المهمة الحالية
extern task_t* task_list;           // قائمة المهام
extern int next_pid;                // معرف المهمة التالي

// دوال إدارة المهام
void init_task_manager();           // تهيئة مدير المهام
task_t* create_task(const char* name, void* entry_point);  // إنشاء مهمة جديدة
void schedule();                    // جدولة المهام
void task_exit(int exit_code);      // إنهاء المهمة
void task_sleep(int ticks);         // إيقاف المهمة مؤقتاً
void task_wake(task_t* task);       // إيقاظ المهمة
task_t* find_task(int pid);         // البحث عن مهمة بالمعرف
void print_task_info();             // طباعة معلومات المهام

// دوال مساعدة
void switch_to_task(task_t* task);  // التبديل إلى مهمة
void save_task_context(task_t* task); // حفظ سياق المهمة
void restore_task_context(task_t* task); // استعادة سياق المهمة

#endif