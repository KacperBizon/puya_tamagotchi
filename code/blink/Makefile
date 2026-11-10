CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
CFLAGS  = -mcpu=cortex-m0plus -mthumb -O2 -Wall -ffreestanding -ffunction-sections -fdata-sections
LDFLAGS = -T linker.ld -nostdlib -Wl,--gc-sections

TARGET  = firmware
OBJS    = main.o startup.o

all: $(TARGET).bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).elf
	pyocd flash -t py32f002bx5 $(TARGET).elf

clean:
	rm -f *.o *.elf *.bin

.PHONY: all flash clean
