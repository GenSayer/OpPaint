# paint4.mak - MSVC4 Makefile for Paint Program
# Supports: x86, MIPS, PowerPC, Alpha on Windows NT
#
# Usage:
#   nmake /f paint4.mak                        (builds for x86)
#   nmake /f paint4.mak TARGET_ARCH=x86
#   nmake /f paint4.mak TARGET_ARCH=mips
#   nmake /f paint4.mak TARGET_ARCH=ppc
#   nmake /f paint4.mak TARGET_ARCH=alpha
#
#   nmake /f paint4.mak DEBUG=1                (debug build)
#   nmake /f paint4.mak clean
#   nmake /f paint4.mak rebuild
#
# Notes:
#   - Requires MSVC4 RISC Edition for non-x86 targets
#   - Windows NT 3.51 or 4.0 required for RISC platforms

# Default architecture
!IFNDEF TARGET_ARCH
TARGET_ARCH = x86
!ENDIF

# Compiler and linker
CC = cl
LINK = link

# Common definitions
DEFINES = /D "WIN32" /D "_WINDOWS"

# Architecture-specific settings
!IF "$(TARGET_ARCH)" == "x86"
# Intel x86
ARCH_CFLAGS = 
ARCH_LFLAGS = /MACHINE:IX86
OUTDIR = obj\x86
!ELSEIF "$(TARGET_ARCH)" == "mips"
# MIPS R4000
ARCH_CFLAGS = /QmipsOb4000
ARCH_LFLAGS = /MACHINE:MIPS
OUTDIR = obj\mips
!ELSEIF "$(TARGET_ARCH)" == "ppc"
# PowerPC 601/603/604
ARCH_CFLAGS = 
ARCH_LFLAGS = /MACHINE:PPC
OUTDIR = obj\ppc
!ELSEIF "$(TARGET_ARCH)" == "alpha"
# DEC Alpha AXP
ARCH_CFLAGS = 
ARCH_LFLAGS = /MACHINE:ALPHA
OUTDIR = obj\alpha
!ELSE
!ERROR Unknown TARGET_ARCH "$(TARGET_ARCH)". Use: x86, mips, ppc, alpha
!ENDIF

# Debug vs Release
!IFDEF DEBUG
CFLAGS_BUILD = /nologo /W3 /Zi /Od /D "_DEBUG"
LFLAGS_BUILD = /nologo /subsystem:windows /debug
!ELSE
CFLAGS_BUILD = /nologo /W3 /O2 /D "NDEBUG"
LFLAGS_BUILD = /nologo /subsystem:windows
!ENDIF

# Combined flags
CFLAGS = $(CFLAGS_BUILD) $(DEFINES) $(ARCH_CFLAGS)
LFLAGS = $(LFLAGS_BUILD) $(ARCH_LFLAGS)

# Libraries
LIBS = user32.lib gdi32.lib comdlg32.lib shell32.lib kernel32.lib

# Object files
OBJ1 = $(OUTDIR)\paint_common.obj
OBJ2 = $(OUTDIR)\paint_win32.obj
OBJECTS = $(OBJ1) $(OBJ2)

# Target executable
TARGET = $(OUTDIR)\paint.exe

# Default target
all: makedirs $(TARGET)
	@echo Build complete: $(TARGET)

# Create output directories
makedirs:
	@if not exist obj mkdir obj
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

# Link executable
$(TARGET): $(OBJ1) $(OBJ2)
	$(LINK) $(LFLAGS) /out:$@ $(OBJECTS) $(LIBS)

# Compile paint_common.c
$(OBJ1): paint_common.c paint.h
	$(CC) $(CFLAGS) /c /Fo$@ paint_common.c

# Compile paint_win32.c
$(OBJ2): paint_win32.c paint.h
	$(CC) $(CFLAGS) /c /Fo$@ paint_win32.c

# Clean current architecture
clean:
	-@if exist $(OUTDIR)\*.obj del $(OUTDIR)\*.obj 2>nul
	-@if exist $(OUTDIR)\*.pdb del $(OUTDIR)\*.pdb 2>nul
	-@if exist $(OUTDIR)\*.ilk del $(OUTDIR)\*.ilk 2>nul
	-@if exist $(TARGET) del $(TARGET) 2>nul

# Clean all architectures
cleanall:
	-@if exist obj\x86 rmdir /s /q obj\x86 2>nul
	-@if exist obj\mips rmdir /s /q obj\mips 2>nul
	-@if exist obj\ppc rmdir /s /q obj\ppc 2>nul
	-@if exist obj\alpha rmdir /s /q obj\alpha 2>nul

# Rebuild
rebuild: clean all

# Build all architectures
allarch:
	$(MAKE) /f paint4.mak TARGET_ARCH=x86
	$(MAKE) /f paint4.mak TARGET_ARCH=mips
	$(MAKE) /f paint4.mak TARGET_ARCH=ppc
	$(MAKE) /f paint4.mak TARGET_ARCH=alpha

# Show configuration
info:
	@echo Target Architecture: $(TARGET_ARCH)
	@echo Output Directory:    $(OUTDIR)
	@echo Compiler Flags:      $(CFLAGS)
	@echo Linker Flags:        $(LFLAGS)
	@echo Target:              $(TARGET)
