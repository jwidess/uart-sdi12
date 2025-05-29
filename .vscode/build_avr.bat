@echo off
setlocal ENABLEEXTENSIONS

REM --- Toolchain setup ---
set TOOLS=C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin
set INCLUDE=C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATmega_DFP\1.7.374\include
set GCCDEV=C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\ATmega_DFP\1.7.374\gcc\dev\atmega2560

REM --- Project structure ---
set INPUT=%~1
for %%F in ("%INPUT%") do (
    set FILE_NAME=%%~nxF
    set FILE_BASE=%%~nF
    set FILE_EXT=%%~xF
)

REM --- Validate .cpp file ---
if /I NOT "%FILE_EXT%"==".cpp" (
    echo [ERROR] Selected file is not a .cpp file: "%FILE_NAME%"
    exit /b 1
)

echo [INFO] Compiling: %INPUT%
echo [INFO] Output: build\%FILE_BASE%.o

REM --- Flags ---
set CFLAGS=-funsigned-char -funsigned-bitfields -DDEBUG -I"%INCLUDE%" -I"libUSDI12" -Og -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -mrelax -g2 -Wall -mmcu=atmega2560 -B "%GCCDEV" -c
set LDFLAGS=-Wl,-Map="build/output.map" -Wl,--start-group -Wl,-lm -Wl,--end-group -Wl,--gc-sections -mrelax -mmcu=atmega2560 -B "%GCCDEV"

REM --- Create build directory ---
if not exist build (
    mkdir build
    echo [INFO] Created build directory
)

REM --- Compile selected file ---
echo [INFO] Compiling main file...
"%TOOLS%\avr-g++.exe" %CFLAGS% "%INPUT%" -o "build\%FILE_BASE%.o"
if %errorlevel% neq 0 goto :error

REM --- Compile library ---
echo [INFO] Compiling libUSDI12...
"%TOOLS%\avr-g++.exe" %CFLAGS% "libUSDI12\USDI12.cpp" -o "build\USDI12.o"
if %errorlevel% neq 0 goto :error

REM --- Link ---
echo [INFO] Linking ELF...
"%TOOLS%\avr-g++.exe" -o "build\output.elf" build\%FILE_BASE%.o build\USDI12.o %LDFLAGS%
if %errorlevel% neq 0 goto :error

REM --- Check ELF was created ---
if not exist build\output.elf (
    echo [ERROR] Linker failed. No ELF output found.
    goto :error
)

REM --- Convert formats ---
echo [INFO] Generating HEX and EEP files...
"%TOOLS%\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures build\output.elf build\output.hex
"%TOOLS%\avr-objcopy.exe" -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 --no-change-warnings -O ihex build\output.elf build\output.eep
"%TOOLS%\avr-objdump.exe" -h -S build\output.elf > build\output.lss
"%TOOLS%\avr-size.exe" build\output.elf

echo [SUCCESS] Build complete!
exit /b 0

:error
echo [FAILURE] Build failed!
exit /b 1
