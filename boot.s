; TFD OS v1.0 - Multiboot Bootloader
; GUARANTEED WORKING VERSION

section .multiboot
align 4

MAGIC    equ 0x1BADB002
FLAGS    equ 0x00000003    ; Bit 0: align modules, Bit 1: memory info
CHECKSUM equ -(MAGIC + FLAGS)

dd MAGIC
dd FLAGS
dd CHECKSUM

; These fields are required when bit 0 is set
dd 0    ; header_addr
dd 0    ; load_addr
dd 0    ; load_end_addr
dd 0    ; bss_end_addr
dd 0    ; entry_addr

section .text
global _start

_start:
    mov esp, stack_top
    push 0              ; Reset EFLAGS
    popf
    
    push ebx            ; Multiboot info pointer
    push eax            ; Magic number 0x2BADB002
    
    extern kernel_main
    call kernel_main
    
    cli
.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 32768
stack_top:
