BUILD_PATH  ?= Binaries
TARGET_NAME ?= YchBoot.efi
TARGET      := $(BUILD_PATH)/$(TARGET_NAME)

ELEVATE_TARGET_NAME ?= Bootelvt.bin
ELEVATE_TARGET      := $(BUILD_PATH)/$(ELEVATE_TARGET_NAME)
ELEVATE_SOURCE      ?= Bootelvt.asm

SOURCE        ?= YchBoot.c
OBJECT        ?= $(BUILD_PATH)/Int/YchBoot.o
SHARED_OBJECT ?= $(BUILD_PATH)/Int/YchBoot.so

GUEST_CFLAGS ?=
CFLAGS ?= -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I/usr/include/efi -I/usr/include/efi/x86_64 -DEFI_FUNCTION_WRAPPER $(GUEST_CFLAGS)
LDFLAGS ?= -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared -Bsymbolic -L/usr/lib -lefi -lgnuefi

CC    ?= gcc
LD    ?= ld
RM    ?= rm
NASM  ?= nasm
ECHO  ?= echo
MKDIR ?= mkdir
OCPY  ?= objcopy

$(TARGET): $(SHARED_OBJECT) $(ELEVATE_TARGET)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Converting \"$<\" to \"$@\"...
	@$(OCPY) -j .text    \
			 -j .sdata   \
			 -j .data    \
			 -j .rodata  \
			 -j .dynamic \
			 -j .dynsym  \
			 -j .rel     \
			 -j .rela    \
			 -j .reloc   \
			 --output-target=efi-app-x86_64 $< $@

$(ELEVATE_TARGET): $(ELEVATE_SOURCE)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Assembling \"$<\" to \"$@\"
	@$(NASM) -f bin $(ASMFLAGS) $< -o $@

$(SHARED_OBJECT): $(OBJECT)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Linking \"$<\" as \"$@\"...
	@$(LD) $< /usr/lib/crt0-efi-x86_64.o $(LDFLAGS) -o $@

$(OBJECT): $(SOURCE)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Boot.Build: Compiling \"$<\" to \"$@\"...
	@$(CC) $< -c $(CFLAGS) -o $@

clean:
	@$(ECHO) Removing build directory \"$(BUILD_PATH)\" recursively...
	@$(RM) -r $(BUILD_PATH)

.PHONY: clean
