all: iso


io.o: kernel/io/io.c kernel/io/io.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/io/io.c -o io.o

	
serial.o: kernel/consol/serial.c kernel/consol/serial.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/consol/serial.c -o serial.o

panic.o: kernel/alarm/panic.c kernel/alarm/panic.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/alarm/panic.c -o panic.o

gdt.o: kernel/gdt/gdt.c kernel/gdt/gdt.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/gdt/gdt.c -o gdt.o

tss.o: kernel/gdt/tss.c kernel/gdt/tss.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/gdt/tss.c -o tss.o

gdt_flush.o: kernel/gdt/gdt_flush.s
	nasm -f elf32 kernel/gdt/gdt_flush.s -o gdt_flush.o

idt.o: kernel/idt/idt.c kernel/idt/idt.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/idt/idt.c -o idt.o

idt_flush.o: kernel/idt/idt_flush.s
	nasm -f elf32 kernel/idt/idt_flush.s -o idt_flush.o

pic.o: kernel/pic/pic.c kernel/pic/pic.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/pic/pic.c -o pic.o

handler_init.o: kernel/handlers/handler_init.c kernel/handlers/handler_init.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/handlers/handler_init.c -o handler_init.o

exception.o: kernel/handlers/exception.c
	i686-elf-gcc -m32 -ffreestanding -c kernel/handlers/exception.c -o exception.o

isr_stub.o: kernel/handlers/isr_stub.s
	nasm -f elf32 kernel/handlers/isr_stub.s -o isr_stub.o

memory_map.o: kernel/memory_map.c kernel/memory_map.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/memory_map.c -o memory_map.o

pmm.o: kernel/pmm/pmm.c kernel/pmm/pmm.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/pmm/pmm.c -o pmm.o

memset.o: kernel/memset.c kernel/memset.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/memset.c -o memset.o

paging.o: kernel/paging/paging.c kernel/paging/paging.h 
	i686-elf-gcc -m32 -ffreestanding -c kernel/paging/paging.c -o paging.o

vmm.o: kernel/vmm/vmm.c kernel/vmm/vmm.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/vmm/vmm.c -o vmm.o

setup.o: kernel/setup.c
	i686-elf-gcc -m32 -ffreestanding -c kernel/setup.c -o setup.o

user.o: kernel/usermode/user.c kernel/usermode/user.h
	i686-elf-gcc -m32 -ffreestanding -c kernel/usermode/user.c -o user.o 

usermode_jmp.o: kernel/usermode/usermode_jmp.s
	nasm -f elf32 kernel/usermode/usermode_jmp.s -o usermode_jmp.o

user_main.bin.o: user_main.bin
	i686-elf-ld -r -b binary -o user_main.bin.o user_main.bin


boot.o: boot.s 
	nasm -f elf32 boot.s -o boot.o

kernel.o: kernel/kernel_main.c
	i686-elf-gcc -m32 -ffreestanding -c kernel/kernel_main.c -o kernel.o

user_main.bin: kernel/usermode/user_main.c kernel/usermode/user.ld
	i686-elf-gcc -m32 -ffreestanding -nostdlib -T kernel/usermode/user.ld -o user_main.elf kernel/usermode/user_main.c
	i686-elf-objcopy -O binary user_main.elf user_main.bin


kernel.elf: boot.o kernel.o linker.ld io.o serial.o panic.o gdt.o tss.o gdt_flush.o idt.o idt_flush.o pic.o handler_init.o exception.o isr_stub.o memory_map.o pmm.o memset.o paging.o vmm.o setup.o   
	i686-elf-ld -T linker.ld -Map=kernel.map -o kernel.elf boot.o kernel.o io.o serial.o panic.o gdt.o tss.o gdt_flush.o idt.o idt_flush.o pic.o handler_init.o exception.o isr_stub.o memory_map.o pmm.o memset.o paging.o vmm.o setup.o   

iso: kernel.elf
	mkdir -p isodir/boot/grub
	cp kernel.elf isodir/boot/kernel.elf
	cp grub/grub.cfg isodir/boot/grub
	grub-mkrescue -o newos.iso isodir	

run: iso
	qemu-system-i386 -cdrom newos.iso -m 4G -serial stdio

clean: 
	rm -f *.o kernel.elf newos.iso
	rm -rf isodir

.PHONY: all iso run clean	