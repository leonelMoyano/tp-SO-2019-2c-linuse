################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MUSE.c \
../src/manejoEstructuras.c \
../src/operaciones.c \
../src/util.c 

OBJS += \
./src/MUSE.o \
./src/manejoEstructuras.o \
./src/operaciones.o \
./src/util.o 

C_DEPS += \
./src/MUSE.d \
./src/manejoEstructuras.d \
./src/operaciones.d \
./src/util.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioNOC" -I"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioNOC/Debug" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


