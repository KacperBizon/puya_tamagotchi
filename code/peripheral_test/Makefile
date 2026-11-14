CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
CFLAGS  = -mcpu=cortex-m0plus -mthumb -O2 -Wall -ffreestanding -ffunction-sections -fdata-sections -fno-tree-loop-distribute-patterns
LDFLAGS = -T linker.ld -nostdlib -Wl,--gc-sections

TARGET  = firmware
OBJS    = main.o startup.o timebase.o i2c.o oled.o buzzer.o

all: $(TARGET).bin

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@ -lgcc
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Flash the ELF: it carries the load address, so no --base-address needed
flash: $(TARGET).elf
	pyocd flash -t py32f002bx5 $(TARGET).elf

clean:
	rm -f *.o *.d *.elf *.bin

-include $(OBJS:.o=.d)

.PHONY: all flash clean
