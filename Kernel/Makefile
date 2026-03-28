BUILD_PATH  ?= Binaries
SOURCE_PATH ?= Source
TARGET_NAME ?= Kernelych.img
TARGET      := $(BUILD_PATH)/$(TARGET_NAME)

SOURCES := $(shell find $(SOURCE_PATH) -name *.c)
OBJECTS := $(SOURCES:%=$(BUILD_PATH)/Int/%.o)

CFLAGS ?= -O2 -ffreestanding -mno-red-zone -fno-pic -fno-stack-protector -Wall -Wextra
LDFLAGS ?= -O2 -ffreestanding -nostdlib -mno-red-zone

RM    ?= rm
CC     = x86_64-elf-gcc
ECHO  ?= echo
MKDIR ?= mkdir

$(TARGET): $(OBJECTS)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Linking final binary \"$@\"...
	@$(CC) $(LDFLAGS) -T Kernel.ld -o $@ $^

$(BUILD_PATH)/Int/%.c.o: %.c
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Compiling \"$<\" to \"$@\"...
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(ECHO) Removing build directory \"$(BUILD_PATH)\" recursively...
	@$(RM) -r $(BUILD_PATH)

.PHONY: clean
