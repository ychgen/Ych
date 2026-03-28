BUILD_PATH  ?= Binaries
SOURCE_PATH ?= Source

TARGET_NAME ?= Imgtool
TARGET      := $(BUILD_PATH)/$(TARGET_NAME)

SOURCES := $(shell find $(SOURCE_PATH) -name *.c)
OBJECTS := $(SOURCES:%=$(BUILD_PATH)/Int/%.o)

ROUTINE_LIB_PATH ?= RoutineLib
ROUTINE_LIB      ?= $(ROUTINE_LIB_PATH)/Binaries/libRoutineLib.a

CFLAGS ?= -I$(ROUTINE_LIB_PATH)/Source -I$(SOURCE_PATH)
LDFLAGS ?= -luuid

RM    ?= rm
CC    ?= gcc
ECHO  ?= echo
MKDIR ?= mkdir

$(TARGET): $(OBJECTS)
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Build.Imgtool: Linking final executable \"$@\"...
	@$(CC) $^ $(ROUTINE_LIB) -o $@ $(LDFLAGS)

$(BUILD_PATH)/Int/%.c.o: %.c
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) Build.Imgtool: Compiling \"$<\" to \"$@\"...
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(ECHO) Removing build directory \"$(BUILD_PATH)\" recursively...
	@$(RM) -r $(BUILD_PATH)

.PHONY: clean
