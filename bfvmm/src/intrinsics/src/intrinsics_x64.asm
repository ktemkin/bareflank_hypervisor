;
; Bareflank Hypervisor
;
; Copyright (C) 2015 Assured Information Security, Inc.
; Author: Rian Quinn        <quinnr@ainfosec.com>
; Author: Brendan Kerrigan  <kerriganb@ainfosec.com>
;
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.
;
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
; Lesser General Public License for more details.
;
; You should have received a copy of the GNU Lesser General Public
; License along with this library; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

global __cpuid_eax:function
global __cpuid_ebx:function
global __cpuid_ecx:function
global __cpuid_edx:function
global __read_rflags:function
global __read_msr:function
global __read_msr32:function
global __write_msr:function
global __read_cr0:function
global __write_cr0:function
global __read_cr3:function
global __write_cr3:function
global __read_cr4:function
global __write_cr4:function
global __read_es:function
global __read_cs:function
global __read_ss:function
global __read_ds:function
global __read_fs:function
global __read_gs:function
global __read_tr:function
global __read_ldtr:function
global __read_rsp:function
global __read_gdt:function
global __read_idt:function
global __outb:function
global __inb:function
global __outw:function
global __inw:function

section .text

; uint32_t cpuid_eax(uint32_t val)
__cpuid_eax:
    push rbx
    push rcx
    push rdx

    mov eax, edi
    cpuid

    pop rdx
    pop rcx
    pop rbx
    ret

; uint32_t cpuid_ebx(uint32_t val)
__cpuid_ebx:
    push rbx
    push rcx
    push rdx

    mov eax, edi
    cpuid
    mov eax, ebx

    pop rdx
    pop rcx
    pop rbx
    ret

; uint32_t cpuid_ecx(uint32_t val)
__cpuid_ecx:
    push rbx
    push rcx
    push rdx

    mov eax, edi
    cpuid
    mov eax, ecx

    pop rdx
    pop rcx
    pop rbx
    ret

; uint32_t cpuid_edx(uint32_t val)
__cpuid_edx:
    push rbx
    push rcx
    push rdx

    mov eax, edi
    cpuid
    mov eax, edx

    pop rdx
    pop rcx
    pop rbx
    ret

; uint64_t read_rflags(void)
__read_rflags:
    pushfq
    pop rax
    ret

; uint64_t __read_msr(uint32_t msr)
__read_msr:
    push rcx
    push rdx

    mov ecx, edi
    rdmsr
    shl rdx, 32
    or rax, rdx

    pop rdx
    pop rcx
    ret

; uint64_t __read_msr32(uint32_t msr)
__read_msr32:
    push rcx
    push rdx

    mov ecx, edi
    rdmsr

    pop rdx
    pop rcx
    ret

; void __write_msr(uint32_t msr, uint64_t val)
__write_msr:
    push rcx
    push rdx

    mov rax, rdi
    mov rdx, rdi
    shr rdx, 32
    wrmsr

    pop rdx
    pop rcx
    ret

; uint64_t __read_cr0(void)
__read_cr0:
    mov rax, cr0
    ret

; void __write_cr0(uint64_t val)
__write_cr0:
    mov cr0, rdi
    ret

; uint64_t __read_cr3(void)
__read_cr3:
    mov rax, cr3
    ret

; void __write_cr3(uint64_t val)
__write_cr3:
    mov cr3, rdi
    ret

; uint64_t __read_cr4(void)
__read_cr4:
    mov rax, cr4
    ret

; void __write_cr4(uint64_t val)
__write_cr4:
    mov cr4, rdi
    ret

; uint16_t __read_es(void)
__read_es:
    mov rax, 0
    mov ax, es
    ret

; uint16_t __read_cs(void)
__read_cs:
    mov rax, 0
    mov ax, cs
    ret

; uint16_t __read_ss(void)
__read_ss:
    mov rax, 0
    mov ax, ss
    ret

; uint16_t __read_ds(void)
__read_ds:
    mov rax, 0
    mov ax, ds
    ret

; uint16_t __read_fs(void)
__read_fs:
    mov rax, 0
    mov ax, fs
    ret

; uint16_t __read_gs(void)
__read_gs:
    mov rax, 0
    mov ax, gs
    ret

; uint16_t __read_tr(void)
__read_tr:
    mov rax, 0
    str ax
    ret

; uint16_t __read_ldtr(void)
__read_ldtr:
    mov rax, 0
    sldt ax
    ret

; uint64_t __read_rsp(void)
__read_rsp:
    mov rax, rsp
    ret

; void __read_gdt(gdt_t *gdt)
__read_gdt:
    sgdt [rdi]
    ret

; void __read_idt(idt_t *idt)
__read_idt:
    sidt [rdi]
    ret

; void __outb(uint16_t val, uint16_t port)
__outb:
	mov ax, di
	mov dx, si
	out dx, al
	ret

; void __outw(uint16_t val, uint16_t port)
__outw:
	mov ax, di
	mov dx, si
	out dx, ax
	ret


; uint8_t __inb(uint16_t port)
__inb:
	mov al, 0
	mov dx, di
	in al, dx
	ret

; uint16_t __inw(uint16_t port)
__inw:
	mov eax, 0
	mov edx, edi
	in ax, dx
	ret

