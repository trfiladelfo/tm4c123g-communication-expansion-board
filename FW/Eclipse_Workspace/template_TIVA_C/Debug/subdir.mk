################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../main.c \
../startup_gcc.c 

OBJS += \
./main.o \
./startup_gcc.o 

C_DEPS += \
./main.d \
./startup_gcc.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-none-eabi-gcc -DPART_TH4C123GH6PM -D__ASSEMBLY__ -DTARGET_IS_TM4C123_RB1 -DARM_MATH_CM4 -I/home/luca/src/stellaris/Tivaware -O0 -g3 -Wall -Wextra -c -fmessage-length=0 -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -ffunction-sections -fdata-sections -mlittle-endian -Wno-missing-field-initializers -std=c99 -g -flto -fno-builtin -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


