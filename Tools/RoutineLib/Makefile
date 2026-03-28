BUILD_PATH  ?= Binaries
SOURCE_PATH ?= Source

TARGET_NAME ?= RoutineLib
TARGET      := $(BUILD_PATH)/lib$(TARGET_NAME).a

SOURCES := $(shell find $(SOURCE_PATH) -name *.c)
OBJECTS := $(SOURCES:%=$(BUILD_PATH)/Int/%.o)

RM    ?= rm
AR    ?= ar
CC    ?= gcc
ECHO  ?= echo
MKDIR ?= mkdir

$(TARGET): $(OBJECTS)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Build.RoutineLib: Linking final static library \"$@\"...
	@$(AR) rcs $@ $^

$(BUILD_PATH)/Int/%.c.o: %.c
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Build.RoutineLib: Compiling \"$<\" to \"$@\"...
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(ECHO) Removing build directory \"$(BUILD_PATH)\" recursively...
	@$(RM) -r $(BUILD_PATH)

.PHONY: clean
