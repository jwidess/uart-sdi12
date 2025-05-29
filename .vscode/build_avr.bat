@echo off
set TOOLS=C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin
set INCLUDE=C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATmega_DFP\1.7.374\include
set GCCDEV=C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATmega_DFP\1.7.374\gcc\dev\atmega2560

set CFLAGS=-funsigned-char -funsigned-bitfields -DDEBUG -I"%INCLUDE%" -I"libUSDI12" -Og -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=atmega2560 -B "%GCCDEV" -c
set LDFLAGS=-Wl,-Map="build/output.map" -Wl,--start-group -Wl,-lm -Wl,--end-group -Wl,--gc-sections -mrelax -mmcu=atmega2560 -B "%GCCDEV"

mkdir build 2>nul

"%TOOLS%\avr-g++.exe" %CFLAGS% UART-SDI12-LibraryTest-V1/UART-SDI12-LibraryTest-V1/main.cpp -o build/main.o
"%TOOLS%\avr-g++.exe" %CFLAGS% libUSDI12/USDI12.cpp -o build/USDI12.o

"%TOOLS%\avr-g++.exe" -o build/output.elf build/main.o build/USDI12.o %LDFLAGS%

"%TOOLS%\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures build/output.elf build/output.hex
"%TOOLS%\avr-objcopy.exe" -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 --no-change-warnings -O ihex build/output.elf build/output.eep
"%TOOLS%\avr-objdump.exe" -h -S build/output.elf > build/output.lss
"%TOOLS%\avr-size.exe" build/output.elf
