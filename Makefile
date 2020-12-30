PREFIX?=arm-none-eabi-
CC=$(PREFIX)gcc
OBJCOPY=$(PREFIX)objcopy
OD=bin

all: main.elf

CFLAGS= --static -nostartfiles -std=c11 -g3 -O0
CFLAGS+= -fno-common -ffunction-sections -fdata-sections
CFLAGS+= -I./libopencm3/include -L./libopencm3/lib
CFLAGS+= -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS+= -DTICKRATE=4000000 -Wdouble-promotion -Wall
CFLAGS+= -DSTM32F4 -lopencm3_stm32f4

LDFLAGS+=-Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group
LDFLAGS+= -T stm32f427.ld


libopencm3/Makefile:
	@echo "Initializing libopencm3 submodule"
	git submodule update --init

libopencm3/lib/libopencm3_stm32f4.a: libopencm3/Makefile
	$(MAKE) $(MAKEFLAGS) -C libopencm3

main.elf: libopencm3/lib/libopencm3_stm32f4.a stm32f4.o test.o main.o decoder.o validation.o
	$(CC) -o main.elf stm32f4.o test.o validation.o decoder.o main.o $(CFLAGS) $(LDFLAGS)

clean:
	-rm *.o main.elf
