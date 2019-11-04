################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/example_alumnos.c \
../src/example_program.c 

OBJS += \
./src/example_alumnos.o \
./src/example_program.o 

C_DEPS += \
./src/example_alumnos.d \
./src/example_program.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -include"/home/utnso/workspace/tp-2019-2c-No-C-Nada/hilolay/bin/libhilolay.so" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


