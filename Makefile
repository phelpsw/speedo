
all : main.hex main.lst

PART=attiny441
PROGPART=t441

#You need a relatively new AVR toolchain to compile this.  It needs support for the 441.
AVRP=~/projects/avr/avr8-gnu-toolchain-linux_x86_64/bin/avr-

#I have not measured the frequency.
CFLAGS=-g -Wall -Os -mmcu=$(PART) -DF_CPU=8000000UL
ASFLAGS:=$(CFLAGS)
CC=avr-gcc

main.hex : main.elf
	$(AVRP)objcopy -j .text -j .data -O ihex main.elf main.hex

main.elf : main.c
	$(AVRP)gcc -I  -g $(CFLAGS)   -mmcu=$(PART) -Wl,-Map,main.map -o $@ main.c

main.lst : main.c
	$(AVRP)gcc -c -g -Wa,-a,-ad $(CFLAGS) main.c > $@

burn : main.hex
	sudo avrdude -c usbtiny -p $(PROGPART) -U flash:w:main.hex

clean :
	rm -f main.hex main.map main.elf

