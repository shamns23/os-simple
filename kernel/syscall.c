#include "syscall.h"
#include "task.h"
#include "memory.h"
#include "interrupt.h"
#include "kernel.h"

// جدول معالجات استدعاءات النظام
syscall_handler_t syscall_table[NR_SYSCALLS];

// إحصائيات استدعاءات النظام
static syscall_stats_t syscall_stats = {0};

// متغير للوقت الحالي (بسيط)
static unsigned int current_time = 0;

// متغير لمعرف المستخدم الحالي
static unsigned int current_uid = 0;

/**
 * تهيئة نظام استدعاءات النظام
 * مستوحى من Linux kernel 0.01
 */
void init_syscalls(void) {
    // تصفير جدول المعالجات
    for (int i = 0; i < NR_SYSCALLS; i++) {
        syscall_table[i] = 0;
    }
    
    // تسجيل المعالجات الأساسية
    register_syscall(SYS_EXIT, sys_exit);
    register_syscall(SYS_FORK, sys_fork);
    register_syscall(SYS_READ, sys_read);
    register_syscall(SYS_WRITE, sys_write);
    register_syscall(SYS_GETPID, sys_getpid);
    register_syscall(SYS_GETUID, sys_getuid);
    register_syscall(SYS_TIME, sys_time);
    register_syscall(SYS_BRK, sys_brk);
    register_syscall(SYS_PAUSE, sys_pause);
    register_syscall(SYS_KILL, sys_kill);
    
    // تصفير الإحصائيات
    syscall_stats.total_calls = 0;
    syscall_stats.successful_calls = 0;
    syscall_stats.failed_calls = 0;
    for (int i = 0; i < NR_SYSCALLS; i++) {
        syscall_stats.calls_per_type[i] = 0;
    }
    
    // إعداد بوابة استدعاء النظام (interrupt 0x80)
    setup_syscall_gate();
    
    // System calls initialized
}

/**
 * تسجيل معالج استدعاء نظام
 */
void register_syscall(int syscall_num, syscall_handler_t handler) {
    if (is_valid_syscall(syscall_num)) {
        syscall_table[syscall_num] = handler;
    }
}

/**
 * معالجة استدعاء النظام الرئيسي
 */
int handle_syscall(syscall_params_t* params) {
    int syscall_num = params->eax;
    int result = -1;
    
    // تحديث الإحصائيات
    syscall_stats.total_calls++;
    
    if (is_valid_syscall(syscall_num) && syscall_table[syscall_num]) {
        syscall_stats.calls_per_type[syscall_num]++;
        
        // استدعاء المعالج
        result = syscall_table[syscall_num](params);
        
        if (result >= 0) {
            syscall_stats.successful_calls++;
        } else {
            syscall_stats.failed_calls++;
        }
    } else {
        syscall_stats.failed_calls++;
        // Invalid system call
    }
    
    return result;
}

/**
 * إعداد بوابة استدعاء النظام
 */
void setup_syscall_gate(void) {
    // تسجيل معالج المقاطعة 0x80 لاستدعاءات النظام
    register_interrupt_handler(0x80, (interrupt_handler_t)syscall_entry);
}

/**
 * sys_exit - إنهاء العملية الحالية
 */
int sys_exit(syscall_params_t* params) {
    int exit_code = params->ebx;
    
    // Process exiting with code logged
    
    // في نظام بسيط، نعود للمجدول
    // في نظام حقيقي، نحرر موارد العملية
    // Process terminating
    
    // جدولة المهمة التالية (إذا كانت متوفرة)
    // schedule();
    
    return 0;
}

/**
 * sys_fork - إنشاء عملية جديدة
 */
int sys_fork(syscall_params_t* params) {
    // تنفيذ مبسط لـ fork
    // Fork system call (simplified)
    
    // في نظام حقيقي، ننشئ نسخة من العملية الحالية
    // هنا نرجع PID وهمي للاختبار
    static int next_pid = 100;
    int child_pid = next_pid++;
    
    // Fork: created child PID logged
    
    // إرجاع PID الطفل للوالد، 0 للطفل
    return child_pid;
}

/**
 * sys_read - قراءة البيانات
 */
int sys_read(syscall_params_t* params) {
    int fd = params->ebx;
    char* buf = (char*)params->ecx;
    int count = params->edx;
    
    // تنفيذ بسيط - قراءة من لوحة المفاتيح فقط
    if (fd == 0) { // stdin
        // في نظام حقيقي، ننتظر إدخال من لوحة المفاتيح
        // Reading from stdin (not implemented)
        return 0;
    }
    
    return -1; // ملف غير مدعوم
}

/**
 * sys_write - كتابة البيانات
 */
int sys_write(syscall_params_t* params) {
    int fd = params->ebx;
    const char* buf = (const char*)params->ecx;
    int count = params->edx;
    
    // تنفيذ بسيط - كتابة للشاشة فقط
    if (fd == 1 || fd == 2) { // stdout أو stderr
        for (int i = 0; i < count && buf[i]; i++) {
            print_char(buf[i]);
        }
        return count;
    }
    
    return -1; // ملف غير مدعوم
}

/**
 * sys_getpid - الحصول على معرف العملية الحالية
 */
int sys_getpid(syscall_params_t* params) {
    // في نظام بسيط، نرجع PID ثابت
    static int current_pid = 1;
    return current_pid;
}

/**
 * sys_getuid - الحصول على معرف المستخدم الحالي
 */
int sys_getuid(syscall_params_t* params) {
    return current_uid; // في نظام بسيط، مستخدم واحد فقط
}

/**
 * sys_time - الحصول على الوقت الحالي
 */
int sys_time(syscall_params_t* params) {
    unsigned int* time_ptr = (unsigned int*)params->ebx;
    
    // تحديث الوقت بناءً على timer ticks
    current_time = get_timer_ticks() / 100; // تقريبي: 100 ticks = 1 ثانية
    
    if (time_ptr) {
        *time_ptr = current_time;
    }
    
    return current_time;
}

/**
 * sys_brk - تغيير حجم الذاكرة
 */
int sys_brk(syscall_params_t* params) {
    void* new_brk = (void*)params->ebx;
    
    // تنفيذ بسيط - استخدام kmalloc/kfree
    // BRK system call (simplified)
    
    // في نظام حقيقي، نغير حدود الذاكرة للعملية
    return (int)new_brk;
}

/**
 * sys_pause - إيقاف مؤقت للعملية
 */
int sys_pause(syscall_params_t* params) {
    // تنفيذ مبسط للإيقاف المؤقت
    // Process paused
    // في نظام حقيقي، نغير حالة العملية ونجدول أخرى
    // schedule();
    return 0;
}

/**
 * sys_kill - إنهاء عملية
 */
int sys_kill(syscall_params_t* params) {
    int pid = params->ebx;
    int signal = params->ecx;
    
    // Kill signal logged
    
    // في نظام بسيط، نبحث عن المهمة ونغير حالتها
    // هذا تنفيذ مبسط جداً
    return 0;
}

/**
 * التحقق من صحة رقم استدعاء النظام
 */
int is_valid_syscall(int syscall_num) {
    return (syscall_num >= 0 && syscall_num < NR_SYSCALLS);
}

/**
 * طباعة إحصائيات استدعاءات النظام
 */
void print_syscall_stats(void) {
    // System Call Statistics logged
    // Total calls, successful, failed counts logged
    // Most used syscalls logged
}

/**
 * الحصول على إحصائيات استدعاءات النظام
 */
syscall_stats_t get_syscall_stats(void) {
    return syscall_stats;
}

// Use external print function from kernel.c
extern void print(const char* str);
extern void print_hex(unsigned int value);
extern void print_char(char c);