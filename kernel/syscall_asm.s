; syscall_asm.s - System call entry point
; Inspired by Linux kernel 0.01

[BITS 32]

section .text
global syscall_entry
global test_syscalls
global syscall_print
extern handle_syscall

; System call entry point (interrupt 0x80)
syscall_entry:
    ; Save all registers
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    push ds
    push es
    
    ; Set kernel segments
    mov ax, 0x10    ; kernel data segment
    mov ds, ax
    mov es, ax
    
    ; Create syscall parameters structure on stack
    push edi        ; 5th parameter
     push esi        ; 4th parameter
     push edx        ; 3rd parameter
     push ecx        ; 2nd parameter
     push ebx        ; 1st parameter
     push eax        ; syscall number
    
    ; Call C syscall handler
    mov eax, esp    ; pass pointer to parameters
     push eax
     call handle_syscall
     add esp, 4      ; clean stack
    
    ; Save return value
    mov ebx, eax
    
    ; Remove parameters from stack
    add esp, 24
    
    ; Restore return value in eax
    mov [esp+32], ebx  ; update saved eax
    
    ; Restore registers
    pop es
    pop ds
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax          ; return value
    
    ; Return from interrupt
    iret

; Test syscalls function
test_syscalls:
    push ebp
    mov ebp, esp
    
    ; test sys_getpid
     mov eax, 20     ; SYS_GETPID
     int 0x80
     
     ; test sys_getuid  
     mov eax, 24     ; SYS_GETUID
     int 0x80
     
     ; test sys_time
     mov eax, 13     ; SYS_TIME
     mov ebx, 0      ; NULL pointer
     int 0x80
    
    mov esp, ebp
    pop ebp
    ret

; Helper function to print message via syscall
syscall_print:
    push ebp
    mov ebp, esp
    push esi
    push edi
    
    ; calculate text length
     mov esi, [ebp+8]  ; text address
     xor edi, edi      ; length counter
    
count_loop:
    cmp byte [esi + edi], 0
    je count_done
    inc edi
    jmp count_loop
    
count_done:
    ; call sys_write
     mov eax, 4       ; SYS_WRITE
     mov ebx, 1       ; stdout
     mov ecx, [ebp+8] ; buffer
     mov edx, edi     ; length
     int 0x80
    
    pop edi
    pop esi
    mov esp, ebp
    pop ebp
    ret

section .data
; test messages
test_msg: db "System call test message", 10, 0
pid_msg: db "Current PID: ", 0
uid_msg: db "Current UID: ", 0
time_msg: db "Current time: ", 0