# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc

# Directory
SOURCE_FOLDER = src
OUTPUT_FOLDER = bin
ISO_NAME      = os2023
DISK_NAME     = storage

# Flags
WARNING_CFLAG = -Wall -Wextra #-Werror
DEBUG_CFLAG   = -ffreestanding -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386


refresh: disk insert-shell run
refresh-gdb: disk insert-shell run-gdb


run-gdb: all
	@qemu-system-i386 -s -drive file=$(OUTPUT_FOLDER)/$(DISK_NAME).$(OUTPUT_FOLDER),format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
run: all
	@qemu-system-i386 -drive file=$(OUTPUT_FOLDER)/$(DISK_NAME).$(OUTPUT_FOLDER),format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso

all: build
build: iso
clean:
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel



kernel:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel_loader.s -o $(OUTPUT_FOLDER)/kernel_loader.o
	@gcc $(CFLAGS) src/framebuffer.c -o bin/framebuffer.o
	@gcc $(CFLAGS) src/stdmem.c -o bin/stdmem.o
	@gcc $(CFLAGS) src/kernel.c -o bin/kernel.o
	@gcc $(CFLAGS) src/gdt.c -o bin/gdt.o
	@gcc $(CFLAGS) src/portio.c -o bin/portio.o
	@gcc $(CFLAGS) src/idt.c -o bin/idt.o
	@gcc $(CFLAGS) src/interrupt.c -o bin/interrupt.o
	@gcc $(CFLAGS) src/keyboard.c -o bin/keyboard.o
	@gcc $(CFLAGS) src/disk.c -o bin/disk.o
	@gcc $(CFLAGS) src/fat32.c -o bin/fat32.o
	@gcc $(CFLAGS) src/paging.c -o bin/paging.o
	@gcc $(CFLAGS) src/frame.c -o bin/frame.o
	@gcc $(CFLAGS) src/syscall-helper.c -o bin/syscall-helper.o
	@gcc $(CFLAGS) src/str.c -o bin/str.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/intsetup.s -o $(OUTPUT_FOLDER)/intsetup.o
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

iso: kernel
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	@cd bin && genisoimage -R -b boot/grub/grub1 -no-emul-boot -boot-load-size 4 -A os -input-charset utf8 -quiet -boot-info-table -o $(ISO_NAME).iso iso
	@rm -r $(OUTPUT_FOLDER)/iso/

disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M

inserter:
	@$(CC) -Wno-builtin-declaration-mismatch -g \
		$(SOURCE_FOLDER)/stdmem.c $(SOURCE_FOLDER)/fat32.c \
		$(SOURCE_FOLDER)/external-inserter.c \
		-o $(OUTPUT_FOLDER)/inserter

user-shell:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/user-entry.s -o user-entry.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/user-shell.c -o $(OUTPUT_FOLDER)/shell-bin/user-shell.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/cd.c -o $(OUTPUT_FOLDER)/shell-bin/cd.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/cp.c -o $(OUTPUT_FOLDER)/shell-bin/cp.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/ls.c -o $(OUTPUT_FOLDER)/shell-bin/ls.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/mkdir.c -o $(OUTPUT_FOLDER)/shell-bin/mkdir.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/cat.c -o $(OUTPUT_FOLDER)/shell-bin/cat.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/mv.c -o $(OUTPUT_FOLDER)/shell-bin/mv.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/rm.c -o $(OUTPUT_FOLDER)/shell-bin/rm.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/whereis.c -o $(OUTPUT_FOLDER)/shell-bin/whereis.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/shell-helper.c -o $(OUTPUT_FOLDER)/shell-bin/shell-helper.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/shell-state.c -o $(OUTPUT_FOLDER)/shell-bin/shell-state.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/shell/systemcall.c -o $(OUTPUT_FOLDER)/shell-bin/systemcall.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/stdmem.c -o $(OUTPUT_FOLDER)/shell-bin/stdmem.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/str.c -o $(OUTPUT_FOLDER)/shell-bin/str.o
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 \
		user-entry.o $(OUTPUT_FOLDER)/shell-bin/*.o -o $(OUTPUT_FOLDER)/shell
	@echo Linking object shell object files and generate flat binary...
	@size --target=binary $(OUTPUT_FOLDER)/shell
	@rm -f $(OUTPUT_FOLDER)/shell-bin/*.o

insert-shell: inserter user-shell
	@echo Inserting shell into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter shell 2 $(DISK_NAME).bin; ./inserter te 2 $(DISK_NAME).bin