# متغيرات المترجم والأدوات
ASM = nasm
CC = gcc
LD = ld
QEMU = qemu-system-x86_64

# خيارات المترجم
ASMFLAGS = -f bin
CFLAGS = -m32 -ffreestanding -fno-stack-protector -nostdlib -c
LDFLAGS = -m elf_i386 -T linker/kernel.ld

# مجلدات المشروع
BOOT_DIR = boot
KERNEL_DIR = kernel
LINKER_DIR = linker
BUILD_DIR = build

# ملفات الهدف
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMG = $(BUILD_DIR)/os.img

# الهدف الافتراضي
all: $(OS_IMG)

# بناء boot sector
$(BOOT_BIN): $(BOOT_DIR)/boot.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# بناء النواة
$(KERNEL_BIN): $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/kernel.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(KERNEL_DIR)/kernel.c -o $(BUILD_DIR)/kernel.o
	$(LD) $(LDFLAGS) $(BUILD_DIR)/kernel.o -o $@

# إنشاء صورة نظام التشغيل (مبسط - قطاع الإقلاع فقط)
$(OS_IMG): $(BOOT_BIN)
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1
	# ملء الصورة لجعلها صورة قرص صحيحة
	dd if=/dev/zero of=$@ bs=512 seek=1 count=1

# إنشاء مجلد البناء
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# تشغيل النظام في QEMU
run: $(OS_IMG)
	$(QEMU) -drive format=raw,file=$(OS_IMG)

# تنظيف الملفات المؤقتة
clean:
	rm -rf $(BUILD_DIR)

# إعادة البناء الكامل
rebuild: clean all

.PHONY: all run clean rebuild