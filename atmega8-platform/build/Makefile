
###############################################################################
# Makefile for the project WRLcopter
###############################################################################

## General Flags
PROJECT = WRLcopter
MCU = atmega8
TARGET = WRLcopter.elf
CC = avr-gcc

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -std=gnu99       -DF_CPU=16000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=WRLcopter.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = WRLcopter.o util_usart.o rc_rx.o TWI_Master.o adc.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) WRLcopter.hex WRLcopter.eep WRLcopter.lss size

## Compile
WRLcopter.o: ../WRLcopter.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

util_usart.o: ../util_usart.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

rc_rx.o: ../rc_rx.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

TWI_Master.o: ../TWI_Master.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

adc.o: ../adc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) WRLcopter.elf dep/* WRLcopter.hex WRLcopter.eep WRLcopter.lss WRLcopter.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

