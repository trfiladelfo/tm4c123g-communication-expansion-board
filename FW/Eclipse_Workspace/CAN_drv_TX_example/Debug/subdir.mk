################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../app_can.c \
../main.c \
../my_uart.c \
../startup_gcc.c 

OBJS += \
./app_can.o \
./main.o \
./my_uart.o \
./startup_gcc.o 

C_DEPS += \
./app_can.d \
./main.d \
./my_uart.d \
./startup_gcc.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-none-eabi-gcc -DPART_TM4C123GH6PM -D__ASSEMBLY__ -DTARGET_IS_TM4C123_RB1 -DARM_MATH_CM4 -I/home/luca/src/stellaris/Tivaware -O0 -g3 -Wall -Wextra -c -fmessage-length=0 -mcpu=cortex-m4 -mthumb -mlittle-endian -Wall -Wextra -Wno-missing-field-initializers -std=c99 -g -ffunction-sections -flto -fno-builtin -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


