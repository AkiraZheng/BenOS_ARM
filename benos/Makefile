ARMGNU ?= aarch64-linux-gnu

board ?= rpi4

ifeq ($(board), rpi3)
COPS += -DCONFIG_BOARD_PI3B
QEMU_FLAGS  += -machine raspi3
else ifeq ($(board), rpi4)
COPS += -DCONFIG_BOARD_PI4B
QEMU_FLAGS  += -machine raspi4
endif

# run EL2 for virtualization
COPS += -DCONFIG_VIRT
ASMOPS += -DCONFIG_VIRT

COPS += -g -Wall -nostdlib -Iinclude -mgeneral-regs-only
ASMOPS += -g -Iinclude 

BUILD_DIR = build
SRC_DIR = src
GUEST_DIR = guestos
GUEST_BUILD_DIR = build/guestos

all : benos.bin

clean :
	rm -rf $(BUILD_DIR) *.bin *.elf *.map 

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c -D__ASSEMBLY__ $< -o $@

$(GUEST_BUILD_DIR)/%_c.o: $(GUEST_DIR)/%.c
	$(CMD_PREFIX)mkdir -p $(GUEST_BUILD_DIR); $(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(GUEST_BUILD_DIR)/%_s.o: $(GUEST_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD $(AOPS) -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
GUEST_C_FILES = $(wildcard $(GUEST_DIR)/*.c)
GUEST_A_FILES = $(wildcard $(GUEST_DIR)/*.S)

OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
OBJ_FILES += $(GUEST_C_FILES:$(GUEST_DIR)/%.c=$(GUEST_BUILD_DIR)/%_c.o)
OBJ_FILES += $(GUEST_A_FILES:$(GUEST_DIR)/%.S=$(GUEST_BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

benos.bin: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld  -Map benos.map -o $(BUILD_DIR)/benos.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/benos.elf -O binary benos.bin
	cp $(BUILD_DIR)/benos.elf benos.elf

QEMU_FLAGS  += -nographic

run:
	qemu-system-aarch64 $(QEMU_FLAGS) -kernel benos.bin
debug:
	qemu-system-aarch64 $(QEMU_FLAGS) -kernel benos.bin -S -s
