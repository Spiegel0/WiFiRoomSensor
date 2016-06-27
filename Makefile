################################################################################
# \brief Makefile which controls the generation of the WiFiRoomSensor
#
# \details The first few lines may be used to configure the setup. The later 
# lines specify the overall build process. Supported targets:
# * all:     Builds everything but avoids any installation
# * clean:   Removes every generated file
# * install: Uploads the source code to the MCU
# * doc:     Generates the documentation
# * binary:  Generates the binary files
# * size:    Computes the size of the output
# 
# \author Michael Spiegel, <michael.h.spiegel@gmail.com>
# 
#  Copyright (C) 2016 Michael Spiegel
# 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; version 2 of the License
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 
################################################################################

# \brief Lists each source file of the project relative to the source directory
SRC_FILES = main.c am2303.c esp8266_transceiver.c system_timer.c
SRC_FILES += esp8266_session.c iec61499_com.c soft_uart.c oscillator.c

# \brief The name of the project
PROJECT = WiFiRoomSensor
# \brief The path to the binary directory
BINDIR = bin
# \brief The path of the source directory
SRCDIR = src
# \brief The documentation directory
DOCDIR = doc/api_doc

# \brief The target microcontroller (GCC's -mmcu option)
MCU = atmega8
# \brief The target microcontroller for avrdude
PROG_MCU = m8
# \brief The CPU frequency
F_CPU = 8000000UL

# The commands in use
CC = avr-gcc
LD = avr-gcc
PROG = avrdude
PSIZE = avr-size
OCOPY = avr-objcopy
ODUMP = avr-objdump
DOX = doxygen
GDB = gdb

# \brief The compiler flags
CC_FLAGS	=  -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Wall -Wstrict-prototypes -O3
CC_FLAGS	+= -frename-registers -fshort-enums -fpack-struct
CC_FLAGS	+= -std=gnu99
# CC_FLAGS	+= -DNDEBUG

# \brief The linker flags
LD_FLAGS	=  -mmcu=$(MCU)

# \brief The programmer flags
PROG_FLAGS = -p$(PROG_MCU) -cavrisp2 -Pusb
# \brief The fuse values to set
PROG_FUSE = -Ulfuse:w:0xe4:m -Uhfuse:w:0xd1:m

# \brief The destination object files
OBJ=$(SRC_FILES:%.c=$(BINDIR)/%.o)

.PHONY: all size clean binary install doc

all: binary

binary: $(BINDIR)/$(PROJECT).elf $(BINDIR)/$(PROJECT).lss size

$(BINDIR):
	mkdir $(BINDIR)
	
$(DOCDIR):
	mkdir -p $(DOCDIR)

$(BINDIR)/%.o: $(SRCDIR)/%.c $(BINDIR) $(SRCDIR)/*.h
	$(CC) $(CC_FLAGS) -c -o $@ $<

$(BINDIR)/$(PROJECT).elf: $(OBJ)
	$(LD) $(LD_FLAGS) -o $@ $^

# \brief Reads the calibration from the connected MCU
$(BINDIR)/calibration.txt:
	$(PROG) $(PROG_FLAGS) -Ucalibration:r:$@:h

# Sets the dynamic variables 
$(BINDIR)/$(PROJECT).mod.elf: $(BINDIR)/$(PROJECT).elf $(BINDIR)/calibration.txt
	cp $< $@
	echo -n "set var (char[4]) oscillator_calibration = { " > $(BINDIR)/conf.batch
	cat $(BINDIR)/calibration.txt | tr -d '\n\r' >> $(BINDIR)/conf.batch
	echo " }" >> $(BINDIR)/conf.batch
	$(GDB) -batch -x $(BINDIR)/conf.batch --write $@

$(BINDIR)/$(PROJECT).hex: $(BINDIR)/$(PROJECT).mod.elf
	$(OCOPY) -R .eeprom -R .fuse -R .lock -R .signature -O ihex $< $@

$(BINDIR)/$(PROJECT).eep: $(BINDIR)/$(PROJECT).mod.elf
	$(OCOPY) -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 \
			-O ihex $< $@

$(BINDIR)/$(PROJECT).lss: $(BINDIR)/$(PROJECT).elf
	$(ODUMP) -h -S $< >$@

size: $(BINDIR)/$(PROJECT).elf
	$(PSIZE) --format=avr --mcu=$(MCU) $<

install: $(BINDIR)/$(PROJECT).hex $(BINDIR)/$(PROJECT).eep
	$(PROG) $(PROG_FLAGS) \
			-Uflash:w:$(BINDIR)/$(PROJECT).hex:a \
			-Ueeprom:w:$(BINDIR)/$(PROJECT).eep:a \
			 $(PROG_FUSE)

doc: $(DOCDIR) $(SRCDIR)/*.c $(SRCDIR)/*.h
	$(DOX) Doxyfile

clean:
	rm -rf $(BINDIR)
	rm -rf $(DOCDIR)
