#ifndef SYSCALL_H
#define SYSCALL_H

#include "task.h"

// رقم استدعاءات النظام - مستوحى من Linux 0.01
#define SYS_EXIT    1   // إنهاء العملية
#define SYS_FORK    2   // إنشاء عملية جديدة
#define SYS_READ    3   // قراءة البيانات
#define SYS_WRITE   4   // كتابة البيانات
#define SYS_OPEN    5   // فتح ملف
#define SYS_CLOSE   6   // إغلاق ملف
#define SYS_WAITPID 7   // انتظار عملية فرعية
#define SYS_CREAT   8   // إنشاء ملف
#define SYS_LINK    9   // إنشاء رابط
#define SYS_UNLINK  10  // حذف ملف
#define SYS_EXECVE  11  // تنفيذ برنامج
#define SYS_CHDIR   12  // تغيير المجلد
#define SYS_TIME    13  // الحصول على الوقت
#define SYS_MKNOD   14  // إنشاء عقدة
#define SYS_CHMOD   15  // تغيير صلاحيات
#define SYS_GETPID  20  // الحصول على معرف العملية
#define SYS_GETUID  24  // الحصول على معرف المستخدم
#define SYS_ALARM   27  // تعيين منبه
#define SYS_PAUSE   29  // إيقاف مؤقت
#define SYS_SYNC    36  // مزامنة البيانات
#define SYS_KILL    37  // إنهاء عملية
#define SYS_MKDIR   39  // إنشاء مجلد
#define SYS_RMDIR   40  // حذف مجلد
#define SYS_PIPE    42  // إنشاء أنبوب
#define SYS_BRK     45  // تغيير حجم الذاكرة
#define SYS_SIGNAL  48  // معالج الإشارات

// الحد الأقصى لعدد استدعاءات النظام
#define NR_SYSCALLS 64

// هيكل معاملات استدعاء النظام
typedef struct {
    unsigned int eax;  // رقم استدعاء النظام
    unsigned int ebx;  // المعامل الأول
    unsigned int ecx;  // المعامل الثاني
    unsigned int edx;  // المعامل الثالث
    unsigned int esi;  // المعامل الرابع
    unsigned int edi;  // المعامل الخامس
} syscall_params_t;

// هيكل إحصائيات استدعاءات النظام
typedef struct {
    unsigned int total_calls;           // إجمالي الاستدعاءات
    unsigned int successful_calls;      // الاستدعاءات الناجحة
    unsigned int failed_calls;          // الاستدعاءات الفاشلة
    unsigned int calls_per_type[NR_SYSCALLS]; // عدد الاستدعاءات لكل نوع
} syscall_stats_t;

// نوع دالة معالج استدعاء النظام
typedef int (*syscall_handler_t)(syscall_params_t* params);

// جدول معالجات استدعاءات النظام
extern syscall_handler_t syscall_table[NR_SYSCALLS];

// دوال تهيئة وإدارة استدعاءات النظام
void init_syscalls(void);
void register_syscall(int syscall_num, syscall_handler_t handler);
int handle_syscall(syscall_params_t* params);
void print_syscall_stats(void);
syscall_stats_t get_syscall_stats(void);

// معالجات استدعاءات النظام الأساسية
int sys_exit(syscall_params_t* params);
int sys_fork(syscall_params_t* params);
int sys_read(syscall_params_t* params);
int sys_write(syscall_params_t* params);
int sys_getpid(syscall_params_t* params);
int sys_getuid(syscall_params_t* params);
int sys_time(syscall_params_t* params);
int sys_brk(syscall_params_t* params);
int sys_pause(syscall_params_t* params);
int sys_kill(syscall_params_t* params);

// دوال مساعدة
int is_valid_syscall(int syscall_num);
void syscall_entry(void);  // نقطة دخول assembly
void setup_syscall_gate(void);

// ماكرو لاستدعاء النظام من مساحة المستخدم
#define SYSCALL0(num) ({ \
    int ret; \
    asm volatile("int $0x80" : "=a"(ret) : "a"(num)); \
    ret; \
})

#define SYSCALL1(num, arg1) ({ \
    int ret; \
    asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1)); \
    ret; \
})

#define SYSCALL2(num, arg1, arg2) ({ \
    int ret; \
    asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2)); \
    ret; \
})

#define SYSCALL3(num, arg1, arg2, arg3) ({ \
    int ret; \
    asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)); \
    ret; \
})

// دوال wrapper للاستدعاءات الشائعة
static inline int getpid(void) {
    return SYSCALL0(SYS_GETPID);
}

static inline int getuid(void) {
    return SYSCALL0(SYS_GETUID);
}

static inline void exit(int status) {
    SYSCALL1(SYS_EXIT, status);
}

static inline int fork(void) {
    return SYSCALL0(SYS_FORK);
}

static inline int write(int fd, const void* buf, int count) {
    return SYSCALL3(SYS_WRITE, fd, (int)buf, count);
}

static inline int read(int fd, void* buf, int count) {
    return SYSCALL3(SYS_READ, fd, (int)buf, count);
}

#endif // SYSCALL_H