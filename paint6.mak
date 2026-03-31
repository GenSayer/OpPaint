# paint6.mak - MSVC6/VS2003 Makefile for Paint Program
# Compatible with Visual C++ 6.0, Visual Studio 2002, Visual Studio 2003
#
# Usage:
#   nmake /f paint6.mak
#   nmake /f paint6.mak DEBUG=1
#   nmake /f paint6.mak clean

CC = cl
LINK = link

# Common definitions
DEFINES = /D "WIN32" /D "_WINDOWS"

# Debug vs Release
!IFDEF DEBUG
CFLAGS = /nologo /W3 /Zi /Od /D "_DEBUG" $(DEFINES)
LFLAGS = /nologo /subsystem:windows /debug
!ELSE
CFLAGS = /nologo /W3 /O2 /D "NDEBUG" $(DEFINES)
LFLAGS = /nologo /subsystem:windows
!ENDIF

LIBS = user32.lib gdi32.lib comdlg32.lib shell32.lib kernel32.lib

OBJECTS = paint_common.obj paint_win32.obj
TARGET = paint.exe

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LINK) $(LFLAGS) /out:$@ $(OBJECTS) $(LIBS)

paint_common.obj: paint_common.c paint.h
	$(CC) $(CFLAGS) /c paint_common.c

paint_win32.obj: paint_win32.c paint.h
	$(CC) $(CFLAGS) /c paint_win32.c

clean:
	-@del *.obj 2>nul
	-@del *.pdb 2>nul
	-@del *.ilk 2>nul
	-@del $(TARGET) 2>nul

rebuild: clean all
