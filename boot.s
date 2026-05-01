; ============================================================
; TFD OS v1.0 "Foxy" - Multiboot Bootloader
; By Sadman | 2026 | GPL v3 License
; ============================================================

section .multiboot
align 4

; Multiboot header - tells GRUB how to load us
MAGIC    equ 0x1BADB002
FLAGS    equ (1 << 0) | (1 << 1)            ; Page-align modules, memory info
CHECKSUM equ -(MAGIC + FLAGS)

dd MAGIC
dd FLAGS
dd CHECKSUM

; No video mode request needed (text mode only)

section .text
global _start

_start:
    ; Set up 32KB stack
    mov esp, stack_top
    
    ; Clear direction flag (forward string ops)
    cld
    
    ; Reset all segment registers
    xor eax, eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Save GRUB multiboot info
    ; EAX = 0x2BADB002 (magic)
    ; EBX = pointer to multiboot info structure
    push ebx        ; Multiboot info pointer
    push eax        ; Magic number
    
    ; Call our kernel!
    extern kernel_main
    call kernel_main
    
    ; If kernel returns, halt forever
    cli
.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 32768      ; 32KB stack space
stack_top:
