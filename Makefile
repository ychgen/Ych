BUILD_PATH    ?= Binaries
BOOT_PATH     ?= Boot
FIRMWARE      ?= FIRMWARE
KERNEL_PATH   ?= Kernel
TOOLS_PATH    ?= Tools
CONTRACT_PATH ?= BootContract/

TOOL_ROUTINE_LIB_PATH ?= $(TOOLS_PATH)/RoutineLib
TOOL_IMGTOOL_PATH ?= $(TOOLS_PATH)/Imgtool
TOOL_MKCONTRACT_PATH ?= $(TOOLS_PATH)/MakeContract

BOOT_BUILD_PATH   ?= $(BUILD_PATH)/$(BOOT_PATH)
KERNEL_BUILD_PATH ?= $(BUILD_PATH)/$(KERNEL_PATH)
TOOLS_BUILD_PATH  ?= $(BUILD_PATH)/$(TOOLS_PATH)

TOOL_ROUTINELIB_BUILD_PATH ?= $(BUILD_PATH)/$(TOOL_ROUTINE_LIB_PATH)
TOOL_IMGTOOL_BUILD_PATH ?= $(BUILD_PATH)/$(TOOL_IMGTOOL_PATH)

BOOTLOADER_NAME  ?= YchBoot.efi
BOOTELVT_NAME    ?= Bootelvt.bin
KERNEL_NAME      ?= Kernelych.bin
OS_IMAGE_NAME    ?= Ych.img
OS_IMAGE_SIZE    ?= 256 # MiB
OS_IMAGE_BLKSZ   ?= 512 # bytes
OS_IMAGE_TARGET  := $(BUILD_PATH)/$(OS_IMAGE_NAME)
OS_EMULATION_RAM ?= 512M

CONTRACT_NAME    ?= BootContract.json
CONTRACT         := $(CONTRACT_PATH)/$(CONTRACT_NAME)

RM      ?= rm
CP      ?= cp
PY      ?= python
MMD     ?= mmd
ECHO    ?= echo
MCPY    ?= mcopy
QEMU    ?= qemu-system-x86_64
IMGTOOL ?= $(TOOL_IMGTOOL_BUILD_PATH)/Imgtool
MKFSFAT ?= mkfs.fat

os-image: $(OS_IMAGE_TARGET)

run: os-image
	@$(RM) $(FIRMWARE)/OVMF_VARS.fd
	@$(CP) $(FIRMWARE)/OVMF_VARS_OriginalCopy.fd $(FIRMWARE)/OVMF_VARS.fd
	@$(QEMU) -monitor stdio -d int,cpu_reset,unimp -cpu max -m $(OS_EMULATION_RAM) \
			 -drive if=pflash,format=raw,unit=0,file=$(FIRMWARE)/OVMF_CODE.fd,readonly=on \
			 -drive if=pflash,format=raw,unit=1,file=$(FIRMWARE)/OVMF_VARS.fd \
			 -drive file=$(OS_IMAGE_TARGET),format=raw,if=none,id=nvme0 \
			 -device nvme,drive=nvme0,serial=nvme-0 \
			 -net none

$(OS_IMAGE_TARGET): tools boot kernel
	@$(IMGTOOL) MakeStandard $@ $(OS_IMAGE_SIZE) $(OS_IMAGE_BLKSZ)
# EFI System Partition on sector 2048 (1MiB mark)
	@$(MKFSFAT) -F 32 --offset=2048 $@
	@$(MMD) -i $@@@1M ::/EFI
	@$(MMD) -i $@@@1M ::/EFI/Boot
	@$(MCPY) -i $@@@1M $(BOOT_BUILD_PATH)/$(BOOTLOADER_NAME) ::/EFI/Boot/Bootx64.efi
# Ych Operating System on sector 206848 (post 100 MiB mark)
	@$(MKFSFAT) -F 32 --offset=206848 $@
	@$(MMD) -i $@@@101M ::/Ych
	@$(MCPY) -i $@@@101M $(KERNEL_BUILD_PATH)/$(KERNEL_NAME) ::/Ych/Krnlych.kr
	@$(MCPY) -i $@@@101M $(BOOT_BUILD_PATH)/$(BOOTELVT_NAME) ::/Ych/Bootelvt.bin

contract: $(TOOL_MKCONTRACT_PATH)/MakeContract.py $(CONTRACT)
	@$(ECHO) Generating Boot Contract...
	@$(PY) $< $(CONTRACT)

tools: routinelib imgtool

boot: contract
	@$(MAKE) -C $(BOOT_PATH) BUILD_PATH=$(abspath $(BOOT_BUILD_PATH)) TARGET_NAME=$(BOOTLOADER_NAME) GUEST_CFLAGS="-I$(abspath .)/BootContract/Include"

kernel: contract
	@$(MAKE) -C $(KERNEL_PATH) BUILD_PATH=$(abspath $(KERNEL_BUILD_PATH)) TARGET_NAME=$(KERNEL_NAME) GUEST_CFLAGS="-I$(abspath .)/BootContract/Include"

routinelib:
	@$(MAKE) -C $(TOOL_ROUTINE_LIB_PATH) BUILD_PATH=$(abspath $(TOOL_ROUTINELIB_BUILD_PATH)) ROUTINE_LIB_PATH=$(abspath $(TOOL_ROUTINE_LIB_PATH))

imgtool: routinelib
	@$(MAKE) -C $(TOOL_IMGTOOL_PATH) BUILD_PATH=$(abspath $(TOOL_IMGTOOL_BUILD_PATH)) \
		ROUTINE_LIB_PATH=$(abspath $(TOOL_ROUTINE_LIB_PATH)) ROUTINE_LIB=$(abspath $(TOOL_ROUTINELIB_BUILD_PATH)/libRoutineLib.a)

clean:
	@$(ECHO) Removing build directory \"$(BUILD_PATH)\" recursively...
	@$(RM) -r $(BUILD_PATH)

.PHONY: os-image run contract boot tools routinelib imgtool clean
