BUILD_PATH   ?= Binaries
SOURCE_PATH  ?= Source
INCLUDE_PATH ?= Include
TARGET_NAME  ?= Kernelych.img
TARGET       := $(BUILD_PATH)/$(TARGET_NAME)

SOURCES := $(shell find $(SOURCE_PATH) \( -name "*.c" -o -name "*.asm" \))
OBJECTS := $(SOURCES:%=$(BUILD_PATH)/Int/%.o)

GUEST_CFLAGS ?=
CFLAGS ?= -O2 -I$(INCLUDE_PATH) -m64 -mcmodel=kernel -ffreestanding -mno-red-zone \
		  -fno-pie -fno-pic -fno-stack-protector -Wall -Wextra -MMD -MP $(GUEST_CFLAGS)
LDFLAGS ?= -nostdlib -no-pie
ASMFLAGS ?= -f elf64

RM    ?= rm
CC     = x86_64-elf-gcc
ECHO  ?= echo
NASM  ?= nasm
MKDIR ?= mkdir

$(TARGET): $(OBJECTS)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Linking final binary \"$@\"...
	@$(CC) $(LDFLAGS) -T Kernel.ld -o $@ $^

$(BUILD_PATH)/Int/%.c.o: %.c
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Compiling \"$<\" to \"$@\"...
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_PATH)/Int/%.asm.o: %.asm
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Assembling \"$<\" to \"$@\"...
	@$(NASM) $(ASMFLAGS) $< -o $@

clean:
	@$(ECHO) Removing build directory \"$(BUILD_PATH)\" recursively...
	@$(RM) -r $(BUILD_PATH)

.PHONY: clean

-include $(OBJECTS:.o=.d)
