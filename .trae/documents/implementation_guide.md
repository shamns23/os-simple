# دليل التنفيذ لنظام التشغيل البسيط

## 1. هيكل المشروع

```
os/
├── boot/
│   ├── boot.asm          # Boot sector الأساسي
│   └── bootloader.asm    # Bootloader المرحلة الثانية
├── kernel/
│   ├── kernel.c          # النواة الأساسية
│   ├── kernel.h          # ملفات الرأس
│   └── vga.c            # تعامل مع VGA
├── linker/
│   └── kernel.ld        # ملف الربط
├── Makefile             # نظام البناء
└── README.md            # وثائق المشروع
```

## 2. متطلبات النظام

### 2.1 الأدوات المطلوبة

* **NASM**: مجمع Assembly

* **GCC**: مترجم C للهدف x86\_64

* **LD**: رابط الملفات

* **QEMU**: محاكي للاختبار

* **Make**: نظام البناء

### 2.2 تثبيت الأدوات على Ubuntu

```bash
sudo apt update
sudo apt install nasm gcc-multilib qemu-system-x86 make
```

## 3. ملفات الكود الأساسية

### 3.1 Boot Sector (boot/boot.asm)

```assembly
[BITS 16]
[ORG 0x7C00]

start:
    ; إعداد المقاطع
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; طباعة رسالة التحميل
    mov si, loading_msg
    call print_string
    
    ; تحميل النواة من القطاع الثاني
    mov ah, 0x02        ; وظيفة قراءة القطاعات
    mov al, 1           ; عدد القطاعات
    mov ch, 0           ; رقم الأسطوانة
    mov cl, 2           ; رقم القطاع
    mov dh, 0           ; رقم الرأس
    mov bx, 0x1000      ; عنوان التحميل
    int 0x13            ; استدعاء BIOS
    
    ; القفز إلى النواة
    jmp 0x1000
    
print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string
done:
    ret
    
loading_msg db 'Loading kernel...', 13, 10, 0

; ملء باقي القطاع بالأصفار
times 510-($-$$) db 0
dw 0xAA55               ; توقيع Boot sector
```

### 3.2 النواة الأساسية (kernel/kernel.c)

```c
#include "kernel.h"

// عنوان مخزن VGA النصي
volatile char* vga_buffer = (volatile char*)0xB8000;

// موضع المؤشر الحالي
int cursor_x = 0;
int cursor_y = 0;

// ألوان VGA
#define VGA_COLOR_WHITE 0x0F
#define VGA_COLOR_BLACK 0x00

void clear_screen() {
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga_buffer[i] = ' ';
        vga_buffer[i + 1] = VGA_COLOR_WHITE;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void print_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    
    int offset = (cursor_y * 80 + cursor_x) * 2;
    vga_buffer[offset] = c;
    vga_buffer[offset + 1] = VGA_COLOR_WHITE;
    
    cursor_x++;
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
}

void print_string(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

// نقطة دخول النواة
void kernel_main() {
    clear_screen();
    print_string("hi");
    
    // حلقة لا نهائية
    while (1) {
        asm volatile("hlt");
    }
}
```

### 3.3 ملف الرأس (kernel/kernel.h)

```c
#ifndef KERNEL_H
#define KERNEL_H

// تعريفات الأنواع الأساسية
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// تصريحات الدوال
void clear_screen();
void print_char(char c);
void print_string(const char* str);
void kernel_main();

#endif
```

### 3.4 ملف الربط (linker/kernel.ld)

```ld
ENTRY(kernel_main)

SECTIONS
{
    . = 0x1000;
    
    .text : {
        *(.text)
    }
    
    .data : {
        *(.data)
    }
    
    .bss : {
        *(.bss)
    }
}
```

### 3.5 Makefile

```makefile
# متغيرات البناء
ASM = nasm
CC = gcc
LD = ld
QEMU = qemu-system-x86_64

# خيارات التجميع
ASMFLAGS = -f bin
CFLAGS = -m32 -ffreestanding -nostdlib -nostartfiles -nodefaultlibs
LDFLAGS = -m elf_i386 -T linker/kernel.ld

# الملفات المصدرية
BOOT_SRC = boot/boot.asm
KERNEL_SRC = kernel/kernel.c

# الملفات الناتجة
BOOT_BIN = build/boot.bin
KERNEL_BIN = build/kernel.bin
OS_IMG = build/os.img

# الهدف الافتراضي
all: $(OS_IMG)

# إنشاء مجلد البناء
build:
	mkdir -p build

# تجميع boot sector
$(BOOT_BIN): $(BOOT_SRC) | build
	$(ASM) $(ASMFLAGS) $< -o $@

# تجميع النواة
$(KERNEL_BIN): $(KERNEL_SRC) | build
	$(CC) $(CFLAGS) -c $< -o build/kernel.o
	$(LD) $(LDFLAGS) build/kernel.o -o $@

# إنشاء صورة النظام
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	# ملء الصورة إلى 1.44MB
	truncate -s 1440K $@

# تشغيل النظام في QEMU
run: $(OS_IMG)
	$(QEMU) -drive format=raw,file=$<

# تنظيف الملفات
clean:
	rm -rf build/

.PHONY: all run clean
```

## 4. خطوات البناء والتشغيل

### 4.1 بناء المشروع

```bash
# الانتقال إلى مجلد المشروع
cd os/

# بناء النظام
make

# تشغيل النظام
make run
```

### 4.2 استكشاف الأخطاء

#### مشاكل شائعة وحلولها:

1. **خطأ في التجميع**: تأكد من تثبيت جميع الأدوات المطلوبة
2. **فشل التحميل**: تحقق من صحة boot sector signature (0xAA55)
3. **عدم ظهور النص**: تأكد من صحة عنوان VGA buffer (0xB8000)

### 4.3 التطوير والتوسيع

يمكن توسيع النظام بإضافة:

* معالج المقاطعات (Interrupt handlers)

* إدارة أكثر تقدماً للذاكرة

* نظام ملفات بسيط

* دعم لوحة المفاتيح

## 5. المراجع والموارد

* [OSDev Wiki](https://wiki.osdev.org/)

* [Intel 64 and IA-32 Architectures Software Developer's Manual](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)

* [NASM Documentation](https://www.nasm.us/docs.php)

* [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler)

