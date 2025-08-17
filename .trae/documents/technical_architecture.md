# الهيكل التقني لنظام التشغيل البسيط

## 1. تصميم المعمارية

```mermaid
graph TD
    A[BIOS/UEFI Firmware] --> B[Boot Sector - 512 bytes]
    B --> C[Bootloader Stage 2]
    C --> D[Kernel Binary]
    D --> E[VGA Text Buffer]
    
    subgraph "Hardware Layer"
        F[x86-64 CPU]
        G[RAM Memory]
        H[VGA Controller]
    end
    
    subgraph "Software Layer"
        B
        C
        D
    end
    
    subgraph "Output Layer"
        E
    end
    
    D --> F
    D --> G
    E --> H
```

## 2. وصف التقنيات

* Frontend: لا يوجد (نظام تشغيل منخفض المستوى)

* Backend: Assembly x86-64 + C

* Build System: GNU Make + GCC + NASM

* Testing: QEMU Emulator

## 3. تعريفات المسارات

| المسار  | الغرض                  |
| ------- | ---------------------- |
| /boot   | ملفات الإقلاع والتحميل |
| /kernel | ملفات النواة الأساسية  |
| /build  | ملفات البناء والتجميع  |

## 4. تعريفات واجهة البرمجة

### 4.1 واجهات النواة الأساسية

إخراج النص

```
void print_string(const char* str)
```

المعاملات:

| اسم المعامل | نوع المعامل  | مطلوب | الوصف              |
| ----------- | ------------ | ----- | ------------------ |
| str         | const char\* | true  | النص المراد طباعته |

الاستجابة:

| اسم المعامل | نوع المعامل | الوصف              |
| ----------- | ----------- | ------------------ |
| void        | void        | لا توجد قيمة إرجاع |

مثال:

```c
print_string("hi");
```

## 5. مخطط معمارية الخادم

```mermaid
graph TD
    A[Hardware Abstraction] --> B[Kernel Core]
    B --> C[Memory Manager]
    B --> D[Display Driver]
    C --> E[Physical Memory]
    D --> F[VGA Buffer]
    
    subgraph "Kernel Space"
        B
        C
        D
    end
    
    subgraph "Hardware"
        E
        F
    end
```

## 6. نموذج البيانات

### 6.1 تعريف نموذج البيانات

```mermaid
erDiagram
    MEMORY_SEGMENT {
        uint64 base_address
        uint64 size
        uint8 type
        uint8 permissions
    }
    
    VGA_CHAR {
        uint8 character
        uint8 color_attribute
    }
    
    KERNEL_INFO {
        uint64 entry_point
        uint64 size
        uint32 checksum
    }
```

### 6.2 لغة تعريف البيانات

جدول خريطة الذاكرة (memory\_map)

```c
// هيكل خريطة الذاكرة
struct memory_segment {
    uint64_t base_address;     // العنوان الأساسي
    uint64_t size;             // حجم القطعة
    uint8_t type;              // نوع الذاكرة (متاحة/محجوزة)
    uint8_t permissions;       // صلاحيات الوصول
};

// مصفوفة VGA للنص
struct vga_char {
    uint8_t character;         // رمز الحرف
    uint8_t color_attribute;   // لون الحرف والخلفية
};

// معلومات النواة
struct kernel_info {
    uint64_t entry_point;      // نقطة دخول النواة
    uint64_t size;             // حجم النواة
    uint32_t checksum;         // مجموع التحقق
};

// ثوابت النظام
#define VGA_TEXT_BUFFER 0xB8000
#define KERNEL_LOAD_ADDRESS 0x100000
#define BOOTLOADER_ADDRESS 0x7C00
```

