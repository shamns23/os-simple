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
$(KERNEL_BIN): $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/task.c $(KERNEL_DIR)/interrupt.c $(KERNEL_DIR)/memory.c $(KERNEL_DIR)/syscall.c $(KERNEL_DIR)/scheduler.c $(KERNEL_DIR)/keyboard.c $(KERNEL_DIR)/kernel.h $(KERNEL_DIR)/task.h $(KERNEL_DIR)/interrupt.h $(KERNEL_DIR)/memory.h $(KERNEL_DIR)/interrupt_asm.s $(KERNEL_DIR)/syscall_asm.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(KERNEL_DIR)/kernel.c -o $(BUILD_DIR)/kernel.o
	$(CC) $(CFLAGS) $(KERNEL_DIR)/task.c -o $(BUILD_DIR)/task.o
	$(CC) $(CFLAGS) $(KERNEL_DIR)/interrupt.c -o $(BUILD_DIR)/interrupt.o
	$(CC) $(CFLAGS) $(KERNEL_DIR)/memory.c -o $(BUILD_DIR)/memory.o
	$(CC) $(CFLAGS) $(KERNEL_DIR)/syscall.c -o $(BUILD_DIR)/syscall.o
	$(CC) $(CFLAGS) $(KERNEL_DIR)/scheduler.c -o $(BUILD_DIR)/scheduler.o
	$(CC) $(CFLAGS) $(KERNEL_DIR)/keyboard.c -o $(BUILD_DIR)/keyboard.o
	nasm -f elf32 $(KERNEL_DIR)/interrupt_asm.s -o $(BUILD_DIR)/interrupt_asm.o
	nasm -f elf32 $(KERNEL_DIR)/syscall_asm.s -o $(BUILD_DIR)/syscall_asm.o
	$(LD) $(LDFLAGS) $(BUILD_DIR)/kernel.o $(BUILD_DIR)/task.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/scheduler.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/interrupt_asm.o $(BUILD_DIR)/syscall_asm.o -o $@

# إنشاء صورة نظام التشغيل (boot sector + kernel)
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1
	# إضافة النواة إلى الصورة
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1
	# ملء باقي الصورة
	dd if=/dev/zero of=$@ bs=512 seek=2 count=1

# إنشاء مجلد البناء
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# تشغيل النظام في QEMU
run: $(OS_IMG)
	$(QEMU) -drive format=raw,file=$(OS_IMG)

# تنظيف الملفات المؤقتة
clean:
	rm -rf $(BUILD_DIR)
	rm -f kernel.o task.o interrupt.o interrupt_asm.o memory.o syscall.o syscall_asm.o scheduler.o keyboard.o kernel.bin

# إعادة البناء الكامل
rebuild: clean all

.PHONY: all run clean rebuild