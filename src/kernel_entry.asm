bits 32
extern _start

KERNEL_VIRTUAL_ADDR		equ 0xC0010000

KERNEL_VIRTUAL_BASE		equ 0xC0000000
KERNEL_PAGE_NUMBER		equ (KERNEL_VIRTUAL_BASE >> 22)

; Early paging initialization

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; 
;
; Virtual address consists is default 32 bit integer 
; xxxxxxxxxx xxxxxxxxxx xxxxxxxxxxxx
; Higher 10 bits - Entry index in page directory
; Next 10 bits   - Entry index in page table
; Lower 12 bits  - Flags(Present, R/W, U/S etc.)
;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; 

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; 
;
; Page table entry consists of 32 bit entry number
; xxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxx
; High 20 bits define frame address
; Low 12 bits define flags
;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; 


; Setup page table no. 0
; Identity-map for the first 4MB of memory
mov ecx, 0						; Current page table entry number
mov edx, (boot_page_table - KERNEL_VIRTUAL_BASE)		; Pointer to page table entry
.page_table_loop:
	cmp ecx, 1024				; Have we already fill all page table?
	je page_table_end			; If so, go to the end
	mov eax, ecx				; Otherwise copy entry number to EAX
	shl eax, 12					; Set frame address
	or eax, 0x3					; Set presence bit
	mov [edx], eax				; Place created entry to the page table
	inc ecx						; Get next page table entry number
	add edx, 4					; Move to the next page table entry
								; (One entry is 4 bytes)
	jmp .page_table_loop

page_table_end:
	; Setup boot page directory
	; Identity-map the first 4MB of memory and
	; virtual kernel page points to the first 4MB of memory
	mov edx, (boot_page_directory - KERNEL_VIRTUAL_BASE)

	mov eax, [edx]
	or eax, 0x3
	sub eax, KERNEL_VIRTUAL_BASE
	mov [edx], eax

	add edx, 4 * KERNEL_PAGE_NUMBER

	mov eax, [edx]
	or eax, 0x3	
	sub eax, KERNEL_VIRTUAL_BASE
	mov [edx], eax

; Load boot page directory
mov ecx, (boot_page_directory - KERNEL_VIRTUAL_BASE)
mov cr3, ecx

; Enable paging by tweaking bit in CR0
mov ecx, cr0
or ecx, 0x80000001
mov cr0, ecx

lea ecx, [start_in_higher_half]
jmp ecx

start_in_higher_half:
	call _start
	jmp $

align 0x1000
boot_page_table:
	times 1024 dd 0

align 0x1000
boot_page_directory:
	dd boot_page_table
	times (KERNEL_PAGE_NUMBER - 1) dd 0
	dd boot_page_table
	times (1024 - KERNEL_PAGE_NUMBER - 1) dd 0
