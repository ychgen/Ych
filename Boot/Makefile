BUILD_PATH  ?= Binaries
TARGET_NAME ?= YchBoot.efi
TARGET      := $(BUILD_PATH)/$(TARGET_NAME)

SOURCE        ?= YchBoot.c
OBJECT        ?= $(BUILD_PATH)/Int/YchBoot.o
SHARED_OBJECT ?= $(BUILD_PATH)/Int/YchBoot.so

CFLAGS ?= -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I/usr/include/efi -I/usr/include/efi/x86_64 -DEFI_FUNCTION_WRAPPER
LDFLAGS ?= -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared -Bsymbolic -L/usr/lib -lefi -lgnuefi

CC    ?= gcc
LD    ?= ld
RM    ?= rm
ECHO  ?= echo
MKDIR ?= mkdir
OCPY  ?= objcopy

$(TARGET): $(SHARED_OBJECT)
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

$(SHARED_OBJECT): $(OBJECT)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Linking \"$<\" as \"$@\"...
	@$(LD) $< /usr/lib/crt0-efi-x86_64.o $(LDFLAGS) -o $@

$(OBJECT): $(SOURCE)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Boot.Build: Compiling \"$<\" to \"$@\"...
	@$(CC) $< -c $(CFLAGS) -o $@
