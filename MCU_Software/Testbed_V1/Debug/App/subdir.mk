################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/GSBP_Basic.c \
../App/GSBP_Basic_Config.c \
../App/MAX1119x.c \
../App/MCU_StateMachine.c \
../App/TCA9535.c \
../App/Testbed.c 

OBJS += \
./App/GSBP_Basic.o \
./App/GSBP_Basic_Config.o \
./App/MAX1119x.o \
./App/MCU_StateMachine.o \
./App/TCA9535.o \
./App/Testbed.o 

C_DEPS += \
./App/GSBP_Basic.d \
./App/GSBP_Basic_Config.d \
./App/MAX1119x.d \
./App/MCU_StateMachine.d \
./App/TCA9535.d \
./App/Testbed.d 


# Each subdirectory must supply rules for building sources it contributes
App/%.o: ../App/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 '-D__weak=__attribute__((weak))' '-D__packed="__attribute__((__packed__))"' -DUSE_HAL_DRIVER -DSTM32F412Rx -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Inc" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/App" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Drivers/CMSIS/Device/ST/STM32F4xx/Include" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Drivers/CMSIS/Include" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Middlewares/ST/STM32_USB_Device_Library/Core/Inc" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc"  -O3 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


