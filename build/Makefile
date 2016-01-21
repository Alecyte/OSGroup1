# -*- makefile -*-

SHELL = /bin/bash
CC = gcc
LD = ld
HDD = 128 # in MB

ifeq ($(strip $(shell command -v $(CC) 2> /dev/null)),)
$(warning *** Compiler ($(CC)) not found. ***)
endif

# Source C files
CSRC := $(wildcard ../*.c)
COBJS := $(patsubst ../%.c,%.o, $(CSRC))

# Compiler and assembler invocation.
WARNINGS = -Wall -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers
CFLAGS = -g -msoft-float -m32 -O -nostdinc -nostdlib -fno-builtin -fno-unit-at-a-time -fno-toplevel-reorder -fno-stack-protector -fno-zero-initialized-in-bss
ASFLAGS = -Wa,--gstabs


all: MBR.bin kernel.bin
	##### Creating null disk of size ${HDD} MB
	@dd if=/dev/zero of=../SOS.dsk count=${HDD} bs=1M status=noxfer >& /dev/null
	##### Writing boot sector
	@dd if=MBR.bin of=../SOS.dsk conv=notrunc status=noxfer >& /dev/null
	##### Writing kernel image
	@dd if=kernel.bin of=../SOS.dsk bs=1 conv=notrunc seek=512 status=noxfer >& /dev/null
	##### Writing user programs
	@./insertprograms ../userprogs/progs.conf
	@echo -e "\nCreated disk image SOS.dsk.\n"

# MBR: text section start = 0x7C00 (first instruction will
#       be in 0x7C00 when running); necessary since our MBR code
#       has a memory-indirect ljmp 
MBR.bin: MBR.o
	##### Compiling MBR
	@$(LD) -N -e0 -Ttext=0x7C00 --oformat binary -m elf_i386 -s -o $@ $<

MBR.o: ../MBR.S
	@$(CC) -c $< -o $@ $(ASFLAGS) $(CFLAGS)

kernel.bin: startup.o $(COBJS)
	@$(LD) -N -T kernel.ld.s startup.o $(COBJS) -m elf_i386 -s -o kernel.bin

startup.o: ../startup.S
	##### Compiling kernel files
	@$(CC) -c $< -o $@ $(ASFLAGS) $(CFLAGS)

%.o: ../%.c
	@$(CC) -c $< -o $@ $(ASFLAGS) $(CFLAGS)

clean:
	@rm *.o *.bin

