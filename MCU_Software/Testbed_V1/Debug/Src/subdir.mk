################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/dma.c \
../Src/gpio.c \
../Src/i2c.c \
../Src/main.c \
../Src/spi.c \
../Src/stm32f4xx_hal_msp.c \
../Src/stm32f4xx_it.c \
../Src/system_stm32f4xx.c \
../Src/tim.c \
../Src/usart.c \
../Src/usb_device.c \
../Src/usbd_cdc_if.c \
../Src/usbd_conf.c \
../Src/usbd_desc.c 

OBJS += \
./Src/dma.o \
./Src/gpio.o \
./Src/i2c.o \
./Src/main.o \
./Src/spi.o \
./Src/stm32f4xx_hal_msp.o \
./Src/stm32f4xx_it.o \
./Src/system_stm32f4xx.o \
./Src/tim.o \
./Src/usart.o \
./Src/usb_device.o \
./Src/usbd_cdc_if.o \
./Src/usbd_conf.o \
./Src/usbd_desc.o 

C_DEPS += \
./Src/dma.d \
./Src/gpio.d \
./Src/i2c.d \
./Src/main.d \
./Src/spi.d \
./Src/stm32f4xx_hal_msp.d \
./Src/stm32f4xx_it.d \
./Src/system_stm32f4xx.d \
./Src/tim.d \
./Src/usart.d \
./Src/usb_device.d \
./Src/usbd_cdc_if.d \
./Src/usbd_conf.d \
./Src/usbd_desc.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o: ../Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 '-D__weak=__attribute__((weak))' '-D__packed="__attribute__((__packed__))"' -DUSE_HAL_DRIVER -DSTM32F412Rx -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Inc" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/App" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Drivers/CMSIS/Device/ST/STM32F4xx/Include" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Drivers/CMSIS/Include" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Middlewares/ST/STM32_USB_Device_Library/Core/Inc" -I"/home/valtin/W_Hasomed/T_Testbed/MCU_Software/Testbed_V1/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc"  -O3 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


