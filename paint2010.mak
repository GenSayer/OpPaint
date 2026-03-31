# paint2010.mak - Visual Studio 2010/2012/2013 Makefile for Paint Program
# Use: nmake /f paint2010.mak
# For debug: nmake /f paint2010.mak DEBUG=1

CC = cl
LINK = link

!IFDEF DEBUG
CFLAGS = /nologo /W3 /Zi /Od /GS /D "WIN32" /D "_DEBUG" /D "_WINDOWS"
LFLAGS = /nologo /subsystem:windows /ENTRY:WinMainCRTStartup /debug /DYNAMICBASE /NXCOMPAT
!ELSE
CFLAGS = /nologo /W3 /O2 /GS /D "WIN32" /D "NDEBUG" /D "_WINDOWS"
LFLAGS = /nologo /subsystem:windows /ENTRY:WinMainCRTStartup /DYNAMICBASE /NXCOMPAT
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
	-del *.obj
	-del *.pdb
	-del *.ilk
	-del $(TARGET)

rebuild: clean all
