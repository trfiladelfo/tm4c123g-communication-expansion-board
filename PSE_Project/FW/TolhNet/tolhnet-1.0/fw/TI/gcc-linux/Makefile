# Makefile to build firmware with gcc on linux
# author: Michele Alessandrini


BASE_DIR = ..
LIBS_DIR = ../../libs
PROGRAMNAME = tolhnet
RELEASE = 1
ifeq ($(RELEASE), 1)
BUILD_DIR = ./Release
else
BUILD_DIR = ./Debug
endif

GCC_EXTRA_OPTIONS = -DPART_TM4C123GH6PM -DTARGET_IS_TM4C123_RB1 -D__ASSEMBLY__ -I$(LIBS_DIR)

LIBRARIES = $(LIBS_DIR)/driverlib/gcc/libdriver.a

SRCS_C = $(shell find $(BASE_DIR) -name '*.c')
SRCS_ASM = $(shell find $(BASE_DIR) -name '*.[Ss]')
SRCS = $(SRCS_C) $(SRCS_ASM)
OBJS = $(addprefix $(BUILD_DIR)/, $(patsubst $(BASE_DIR)/%, %, $(addsuffix .o, $(basename $(SRCS)))))
DEPS = $(addprefix $(BUILD_DIR)/, $(patsubst $(BASE_DIR)/%, %, $(addsuffix .d, $(basename $(SRCS_C)))))


# You need to download the official ARM gcc for Cortex
#  at https://launchpad.net/gcc-arm-embedded
GCC_ARM_PATH = /usr/bin
COMPILER = $(GCC_ARM_PATH)/arm-none-eabi-gcc
LINKER = $(GCC_ARM_PATH)/arm-none-eabi-ld
OBJCOPY = $(GCC_ARM_PATH)/arm-none-eabi-objcopy
OBJDUMP = $(GCC_ARM_PATH)/arm-none-eabi-objdump
GDB = $(GCC_ARM_PATH)/arm-none-eabi-gdb

CXXFLAGS = -mcpu=cortex-m4 -mthumb -mlittle-endian \
	-Wall -Wextra -Wno-missing-field-initializers -std=c99 -g \
	-ffunction-sections -flto -fno-builtin

ifeq ($(RELEASE), 1)
CXXFLAGS += -O2
endif

CXXFLAGS += $(GCC_EXTRA_OPTIONS)

LINKSCRIPT = tm4c123gh6pm.ld
GCCLINKFLAGS = -static -Wl,--gc-sections -flto -Wl,--entry=ResetISR -nostartfiles
ifeq ($(RELEASE), 1)
GCCLINKFLAGS += -Wl,--strip-debug
endif


TARGETS = $(BUILD_DIR)/$(PROGRAMNAME).out $(BUILD_DIR)/$(PROGRAMNAME).srec $(BUILD_DIR)/$(PROGRAMNAME).bin $(BUILD_DIR)/$(PROGRAMNAME).out.dump


# targets

all: $(TARGETS)

clean:
	rm -f $(TARGETS) $(OBJS) $(DEPS) $(BUILD_DIR)/*.map
	find $(BUILD_DIR) -depth -type d -empty -exec rmdir {} \;

sync:
	cp -a ../../../master/network/errors.h  ../Network/errors.h
	cp -a ../../../master/network/network.h ../Network/network.h
	sed -e "s-[A-Za-z_][A-Za-z0-9_]*::--g" ../../../master/network/network.cpp > ../Network/network.c

diff:
	sed -e "s-[A-Za-z_][A-Za-z0-9_]*::--g" ../../../master/network/network.cpp > ../Network/network_from_cpp
	kompare ../Network/network.c ../Network/network_from_cpp

# rules

include $(DEPS)

$(BUILD_DIR)/%.o: $(BASE_DIR)/%.c
	-mkdir -p $(dir $@)
	$(COMPILER) $(CXXFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(BASE_DIR)/%.S
	-mkdir -p $(dir $@)
	$(COMPILER) $(CXXFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(BASE_DIR)/%.s
	-mkdir -p $(dir $@)
	$(COMPILER) $(CXXFLAGS) -o $@ -c $<



# automatic C dependency handling
$(BUILD_DIR)/%.d: $(BASE_DIR)/%.c
	-mkdir -p $(dir $@)
	$(COMPILER) -MM -MT $(addsuffix .o, $(basename $@)) $(CXXFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@



%.srec: %.out
	$(OBJCOPY) -O srec  $< $@

%.bin: %.out
	$(OBJCOPY) -O binary  $< $@

%.out.dump: %.out
	$(OBJDUMP) -dxgsSt $< > $@



$(BUILD_DIR)/%.out: $(OBJS)
	$(COMPILER) -T $(LINKSCRIPT) $(CXXFLAGS) $(GCCLINKFLAGS) -o $@ $^ $(LIBRARIES) -Wl,-Map=$@.map


flash: $(BUILD_DIR)/$(PROGRAMNAME).bin
	lm4flash -v $<

flash1: $(BUILD_DIR)/$(PROGRAMNAME).bin $(BASE_DIR)/mac_addresses.bin
	dd if=$(BASE_DIR)/mac_addresses.bin of=$< bs=8 count=1 seek=78 conv=notrunc skip=0
	lm4flash -v -s 0E2089D2 $<

flash2: $(BUILD_DIR)/$(PROGRAMNAME).bin $(BASE_DIR)/mac_addresses.bin
	dd if=$(BASE_DIR)/mac_addresses.bin of=$< bs=8 count=1 seek=78 conv=notrunc skip=1
	lm4flash -v -s 0E209239 $<

flash3: $(BUILD_DIR)/$(PROGRAMNAME).bin $(BASE_DIR)/mac_addresses.bin
	dd if=$(BASE_DIR)/mac_addresses.bin of=$< bs=8 count=1 seek=78 conv=notrunc skip=2
	lm4flash -v -s 0E209B49 $<

flash4: $(BUILD_DIR)/$(PROGRAMNAME).bin $(BASE_DIR)/mac_addresses.bin
	dd if=$(BASE_DIR)/mac_addresses.bin of=$< bs=8 count=1 seek=78 conv=notrunc skip=3
	lm4flash -v -s 0E2096F6 $<

dbg: $(BUILD_DIR)/$(PROGRAMNAME).out .gdbinit
	$(GDB) -tui $(BUILD_DIR)/$(PROGRAMNAME).out



.PHONY: all clean flash dbg
.SECONDARY: $(OBJS)
FORCE:

