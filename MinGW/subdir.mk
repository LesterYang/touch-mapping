################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../main.c \
../qUtils.c \
../tm.c \
../tmInput.c \
../tmMapping.c \
../tm_test.c 

OBJS += \
./main.o \
./qUtils.o \
./tm.o \
./tmInput.o \
./tmMapping.o \
./tm_test.o 

C_DEPS += \
./main.d \
./qUtils.d \
./tm.d \
./tmInput.d \
./tmMapping.d \
./tm_test.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	mingw32-gcc -I"D:\MinGW\include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


