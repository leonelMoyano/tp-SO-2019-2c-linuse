################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/pruebaMuse.c 

OBJS += \
./src/pruebaMuse.o 

C_DEPS += \
./src/pruebaMuse.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I/home/utnso/workspace/tp-2019-2c-No-C-Nada/libMUSE/libMUSE -I/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioNOC -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


