################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: template_TIVA_C.elf

# Tool invocations
template_TIVA_C.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Linker'
	arm-none-eabi-gcc -nostartfiles -L/home/luca/src/stellaris/Tivaware/driverlib/gcc -Wl,--static,--gc-sections,-T../tm4c123gh6pm.ld -mthumb -mcpu=cortex-m4 -Wl,--entry=ResetISR -o "template_TIVA_C.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) template_TIVA_C.elf
	-@echo ' '

post-build:
	-@echo 'Generate binary file from elf file'
	-arm-none-eabi-objcopy -O binary template_TIVA_C.elf template_TIVA_C.bin
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
