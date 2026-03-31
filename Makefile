# Makefile for Paint Program (X11/Motif)
# Compatible with GCC 2.95.x and newer, plus various vendor compilers
#
# Supported platforms:
#   Linux, FreeBSD, NetBSD, OpenBSD, DragonFly BSD
#   Solaris, HP-UX, AIX, IRIX
#   SCO OpenServer, SCO UnixWare
#   Tru64 UNIX (Digital UNIX / OSF/1)
#   macOS/Darwin, Cygwin, MSYS2
#   Various historical UNIX systems
#
# GCC compatibility: 2.7.x, 2.8.x, 2.95.x, 3.x, 4.x, and newer

CC = gcc
CFLAGS = -ansi -pedantic -Wall -O2
INCLUDES = -I/usr/include -I/usr/X11R6/include -I/usr/include/X11
LIBDIRS = -L/usr/lib -L/usr/X11R6/lib -L/usr/lib/X11

# Motif libraries - order matters!
LIBS = -lXm -lXt -lX11 -lm

SOURCES = paint_common.c paint_x11.c
OBJECTS = paint_common.o paint_x11.o
TARGET = paint

.PHONY: all clean debug install help
.PHONY: linux linux-lesstif linux-old linux64 linux-local linux-debian linux-redhat linux-slackware
.PHONY: freebsd freebsd-ports netbsd openbsd dragonfly
.PHONY: solaris solaris-gcc solaris-omf solaris7 solaris8 solaris10-sparc64 solaris10-amd64
.PHONY: solaris-gcc-sparc64 solaris-gcc-amd64 openindiana
.PHONY: hpux hpux9 hpux10 hpux11 hpux11-64 hpux-gcc hpux10-gcc hpux11-gcc hpux11-gcc-64
.PHONY: hpux11i-ia64 hpux11i-ia64-gcc hpux11i-ia64-32 hpux11i-ia64-gcc-32
.PHONY: aix aix4 aix5 aix-64 aix-gcc aix-gcc-64 aix-threads
.PHONY: irix irix-n32 irix-64 irix-gcc irix-gcc-n32
.PHONY: sco-osr5 sco-osr5-gcc sco-osr6 sco-osr6-gcc
.PHONY: unixware unixware2 unixware7 unixware-gcc unixware2-gcc
.PHONY: unixware7-x11r5 unixware7-x11r6 unixware-alt
.PHONY: tru64 tru64-cde tru64-motif12 tru64-gcc digitalu digitalu-gcc osf1 osf1-gcc
.PHONY: motif12 svr4 svr3 svr4-gcc svr3-gcc
.PHONY: generic-gcc gcc-old gcc-27 gcc-28 gcc-295 gcc-3 gcc-4 egcs
.PHONY: static 32bit 64bit profile gdb minimal knr
.PHONY: macos macos-homebrew cygwin msys2
.PHONY: clang clang-linux clang-freebsd icc icc-linux pgcc sunstudio sunstudio-linux
.PHONY: cross-arm cross-ppc cross-mips
.PHONY: analyze valgrind sanitize coverage
.PHONY: nextstep ultrix sunos4 aux xenix interactive dynix dgux ncr pyramid
.PHONY: sysv88 bull sinix unisys concurrent stratus tandem
.PHONY: qnx4 qnx6 lynxos vxworks

# =============================================================================
# Default target
# =============================================================================

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $(OBJECTS) $(LIBS)

paint_common.o: paint_common.c paint.h
	$(CC) $(CFLAGS) $(INCLUDES) -c paint_common.c

paint_x11.o: paint_x11.c paint.h
	$(CC) $(CFLAGS) $(INCLUDES) -c paint_x11.c

debug: CFLAGS = -ansi -pedantic -Wall -g -DDEBUG
debug: clean $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET) core *.core a.out

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	chmod 755 /usr/local/bin/$(TARGET)

# =============================================================================
# Linux variants
# =============================================================================

# Linux with OpenMotif (most common modern setup)
linux:
	$(CC) -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Linux with LessTif
linux-lesstif:
	$(CC) -ansi -Wall -O2 \
		-I/usr/X11R6/include \
		-I/usr/X11R6/include/Xm \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lXext -lm

# Linux with old GCC (2.7.x - 2.95.x) and X11R5/Motif 1.2
linux-old:
	$(CC) -ansi -pedantic -Wall -O2 \
		-I/usr/X11R6/include \
		-I/usr/include/Xm \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lXext -lm

# Modern Linux 64-bit with OpenMotif
linux64:
	gcc -m64 -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib64 -L/usr/lib/x86_64-linux-gnu \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Linux with Motif from /usr/local (custom install)
linux-local:
	gcc -ansi -Wall -O2 \
		-I/usr/local/include \
		-I/usr/include/X11 \
		-L/usr/local/lib \
		-L/usr/lib \
		-Wl,-rpath,/usr/local/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Debian/Ubuntu specific paths
linux-debian:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-I/usr/include/Xm \
		-L/usr/lib/x86_64-linux-gnu \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Red Hat / CentOS / Fedora specific
linux-redhat:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib64 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Slackware Linux
linux-slackware:
	gcc -ansi -Wall -O2 \
		-I/usr/X11R6/include \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# BSD variants
# =============================================================================

# FreeBSD
freebsd:
	$(CC) -ansi -Wall -O2 \
		-I/usr/local/include \
		-I/usr/X11R6/include \
		-L/usr/local/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# FreeBSD with ports OpenMotif
freebsd-ports:
	gcc -ansi -Wall -O2 \
		-I/usr/local/include \
		-I/usr/local/include/X11 \
		-L/usr/local/lib \
		-Wl,-rpath,/usr/local/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# NetBSD
netbsd:
	$(CC) -ansi -Wall -O2 \
		-I/usr/X11R7/include \
		-I/usr/pkg/include \
		-L/usr/X11R7/lib \
		-L/usr/pkg/lib \
		-Wl,-R/usr/X11R7/lib:/usr/pkg/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# OpenBSD
openbsd:
	$(CC) -ansi -Wall -O2 \
		-I/usr/X11R6/include \
		-I/usr/local/include \
		-L/usr/X11R6/lib \
		-L/usr/local/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# DragonFly BSD
dragonfly:
	gcc -ansi -Wall -O2 \
		-I/usr/local/include \
		-I/usr/pkg/include \
		-L/usr/local/lib \
		-L/usr/pkg/lib \
		-Wl,-rpath,/usr/local/lib:/usr/pkg/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Sun/Oracle Solaris
# =============================================================================

# Solaris 2.x/SunOS 5.x with CDE Motif (using Sun cc)
solaris:
	cc -Xa -O \
		-I/usr/dt/include \
		-I/usr/openwin/include \
		-L/usr/dt/lib \
		-L/usr/openwin/lib \
		-R/usr/dt/lib:/usr/openwin/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lgen -lsocket -lnsl

# Solaris with GCC
solaris-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/dt/include \
		-I/usr/openwin/include \
		-L/usr/dt/lib \
		-L/usr/openwin/lib \
		-Wl,-R,/usr/dt/lib:/usr/openwin/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lgen -lsocket -lnsl

# Solaris 10/11 with OpenMotif
solaris-omf:
	cc -Xa -O \
		-I/usr/include \
		-I/usr/openwin/include \
		-L/usr/lib \
		-L/usr/openwin/lib \
		-R/usr/lib:/usr/openwin/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lsocket -lnsl

# Solaris 2.6/7 with Sun cc
solaris7:
	cc -Xa -O \
		-I/usr/dt/include \
		-I/usr/openwin/include \
		-L/usr/dt/lib \
		-L/usr/openwin/lib \
		-R/usr/dt/lib:/usr/openwin/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lgen -lsocket -lnsl

# Solaris 8/9 with Sun cc
solaris8:
	cc -Xa -O \
		-I/usr/dt/include \
		-I/usr/openwin/include \
		-L/usr/dt/lib \
		-L/usr/openwin/lib \
		-R/usr/dt/lib:/usr/openwin/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lgen -lsocket -lnsl

# Solaris 10 SPARC 64-bit
solaris10-sparc64:
	cc -Xa -xarch=v9 -O \
		-I/usr/dt/include \
		-I/usr/openwin/include \
		-L/usr/dt/lib/64 \
		-L/usr/openwin/lib/64 \
		-R/usr/dt/lib/64:/usr/openwin/lib/64 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lsocket -lnsl

# Solaris 10 x86 64-bit
solaris10-amd64:
	cc -Xa -xarch=amd64 -O \
		-I/usr/dt/include \
		-I/usr/openwin/include \
		-L/usr/dt/lib/64 \
		-L/usr/openwin/lib/64 \
		-R/usr/dt/lib/64:/usr/openwin/lib/64 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lsocket -lnsl

# Solaris with GCC 64-bit SPARC
solaris-gcc-sparc64:
	gcc -m64 -ansi -Wall -O2 \
		-I/usr/dt/include \
		-I/usr/openwin/include \
		-L/usr/dt/lib/64 \
		-L/usr/openwin/lib/64 \
		-Wl,-R,/usr/dt/lib/64:/usr/openwin/lib/64 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lgen -lsocket -lnsl

# Solaris with GCC 64-bit x86
solaris-gcc-amd64:
	gcc -m64 -ansi -Wall -O2 \
		-I/usr/dt/include \
		-I/usr/openwin/include \
		-L/usr/dt/lib/64 \
		-L/usr/openwin/lib/64 \
		-Wl,-R,/usr/dt/lib/64:/usr/openwin/lib/64 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lgen -lsocket -lnsl

# OpenIndiana / illumos
openindiana:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-I/usr/X11/include \
		-L/usr/lib \
		-L/usr/X11/lib \
		-R/usr/lib:/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lsocket -lnsl

# =============================================================================
# HP-UX
# =============================================================================

# HP-UX 10.x with HP ANSI C and Motif 1.2
hpux:
	cc -Aa +O2 \
		-I/usr/include/Motif1.2 \
		-I/usr/include/X11R5 \
		-L/usr/lib/Motif1.2 \
		-L/usr/lib/X11R5 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 9.x (older, PA-RISC 1.x)
hpux9:
	cc -Aa +O2 \
		-I/usr/include/Motif1.1 \
		-I/usr/include/X11R4 \
		-L/usr/lib/Motif1.1 \
		-L/usr/lib/X11R4 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 10.x explicit target
hpux10:
	cc -Aa +O2 \
		-I/usr/include/Motif1.2 \
		-I/usr/include/X11R5 \
		-L/usr/lib/Motif1.2 \
		-L/usr/lib/X11R5 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 11.x with Motif 2.1
hpux11:
	cc -Aa +O2 \
		-I/usr/include/Motif2.1 \
		-I/usr/include/X11R6 \
		-L/usr/lib/Motif2.1 \
		-L/usr/lib/X11R6 \
		-Wl,+s \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 11.x 64-bit with HP cc (PA-RISC)
hpux11-64:
	cc -Aa +O2 +DA2.0W +DS2.0 \
		-I/usr/include/Motif2.1 \
		-I/usr/include/X11R6 \
		-L/usr/lib/Motif2.1/pa20_64 \
		-L/usr/lib/X11R6/pa20_64 \
		-Wl,+s \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 10.x with GCC
hpux-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include/Motif1.2 \
		-I/usr/include/X11R5 \
		-L/usr/lib/Motif1.2 \
		-L/usr/lib/X11R5 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 10.x with GCC (explicit)
hpux10-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include/Motif1.2 \
		-I/usr/include/X11R5 \
		-L/usr/lib/Motif1.2 \
		-L/usr/lib/X11R5 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 11.x with GCC
hpux11-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include/Motif2.1 \
		-I/usr/include/X11R6 \
		-L/usr/lib/Motif2.1 \
		-L/usr/lib/X11R6 \
		-Wl,-E \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 11.x 64-bit with GCC (PA-RISC)
hpux11-gcc-64:
	gcc -mlp64 -ansi -Wall -O2 \
		-I/usr/include/Motif2.1 \
		-I/usr/include/X11R6 \
		-L/usr/lib/Motif2.1/pa20_64 \
		-L/usr/lib/X11R6/pa20_64 \
		-Wl,-E \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 11i on Itanium (IA-64) with HP cc
hpux11i-ia64:
	cc -Aa +O2 +DD64 \
		-I/usr/include/Motif2.1 \
		-I/usr/include/X11R6 \
		-L/usr/lib/hpux64/Motif2.1 \
		-L/usr/lib/hpux64/X11R6 \
		-Wl,+s \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 11i on Itanium with GCC
hpux11i-ia64-gcc:
	gcc -mlp64 -ansi -Wall -O2 \
		-I/usr/include/Motif2.1 \
		-I/usr/include/X11R6 \
		-L/usr/lib/hpux64/Motif2.1 \
		-L/usr/lib/hpux64/X11R6 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 11i 32-bit on Itanium with HP cc
hpux11i-ia64-32:
	cc -Aa +O2 +DD32 \
		-I/usr/include/Motif2.1 \
		-I/usr/include/X11R6 \
		-L/usr/lib/hpux32/Motif2.1 \
		-L/usr/lib/hpux32/X11R6 \
		-Wl,+s \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# HP-UX 11i 32-bit on Itanium with GCC
hpux11i-ia64-gcc-32:
	gcc -milp32 -ansi -Wall -O2 \
		-I/usr/include/Motif2.1 \
		-I/usr/include/X11R6 \
		-L/usr/lib/hpux32/Motif2.1 \
		-L/usr/lib/hpux32/X11R6 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# IBM AIX
# =============================================================================

# AIX 4.x/5.x with IBM XL C
aix:
	xlc -O2 -qlanglvl=ansi \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# AIX 4.x specific
aix4:
	xlc -O2 -qlanglvl=ansi \
		-I/usr/include \
		-I/usr/lpp/X11/include \
		-L/usr/lib \
		-L/usr/lpp/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# AIX 5.x specific
aix5:
	xlc -O2 -qlanglvl=ansi \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lpthread

# AIX 5.3 / 6.1 / 7.x 64-bit with XL C
aix-64:
	xlc -q64 -O2 -qlanglvl=ansi \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# AIX with GCC
aix-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# AIX with GCC 64-bit
aix-gcc-64:
	gcc -maix64 -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# AIX with thread support
aix-threads:
	xlc_r -O2 -qlanglvl=ansi \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lpthread

# =============================================================================
# SGI IRIX
# =============================================================================

# SGI IRIX 5.x/6.x with MIPSpro C (o32 ABI)
irix:
	cc -ansi -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# IRIX with N32 ABI (IRIX 6.x)
irix-n32:
	cc -ansi -n32 -O2 \
		-I/usr/include \
		-L/usr/lib32 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# IRIX with 64-bit ABI (IRIX 6.x)
irix-64:
	cc -ansi -64 -O2 \
		-I/usr/include \
		-L/usr/lib64 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# IRIX with GCC
irix-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm
		
# IRIX with GCC with n32 ABI
irix-gcc-n32:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib32 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# SCO OpenServer 5.x (SVR3-based)
# =============================================================================

# SCO OpenServer 5.0.x with SCO Development System
sco-osr5:
	cc -b elf -O \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lm -lsocket -lgen -lPW

# SCO OpenServer 5.0.x with GCC
sco-osr5-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lgen

# SCO OpenServer 6.x (SVR5/UnixWare-based)
sco-osr6:
	cc -O \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lm -lsocket -lnsl -lgen

# SCO OpenServer 6.x with GCC
sco-osr6-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lnsl -lgen

# =============================================================================
# SCO UnixWare 2.x (SVR4.2-based)
# =============================================================================

# UnixWare 2.x with standard compiler
unixware2:
	cc -O \
		-I/usr/X/include \
		-L/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lm -lsocket -lnsl -lgen

# UnixWare 2.x with GCC
unixware2-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/X/include \
		-I/usr/X/include/Xm \
		-L/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lnsl -lgen

# =============================================================================
# SCO UnixWare 7.x (SVR5-based)
# =============================================================================

# UnixWare 7.x with UDK compiler
unixware:
	cc -O \
		-I/usr/include \
		-I/usr/X/include \
		-L/usr/lib \
		-L/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lnsl -lgen

# UnixWare 7.x alternate (Motif in different location)
unixware7:
	cc -O \
		-I/usr/X/include \
		-I/usr/X/include/Xm \
		-L/usr/X/lib \
		-R/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lnsl -lgen -lc

# UnixWare 7.x with GCC
unixware-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/X/include \
		-L/usr/X/lib \
		-Wl,-R,/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lnsl -lgen

# UnixWare 7.x with older X11 location
unixware7-x11r5:
	cc -O \
		-I/usr/X11R5/include \
		-L/usr/X11R5/lib \
		-R/usr/X11R5/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lm -lsocket -lnsl -lgen

# UnixWare 7.x with X11R6
unixware7-x11r6:
	cc -O \
		-I/usr/X11R6/include \
		-L/usr/X11R6/lib \
		-R/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lnsl -lgen

# UnixWare alternate with libs in /usr/lib
unixware-alt:
	cc -O \
		-I/usr/X/include \
		-L/usr/X/lib \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lnsl -lgen

# =============================================================================
# Tru64 UNIX / Digital UNIX / OSF/1 (Alpha)
# =============================================================================

# Tru64 UNIX 4.x/5.x with DEC C
tru64:
	cc -std1 -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-L/usr/shlib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -ldnet_stub

# Tru64 UNIX 5.x with CDE Motif
tru64-cde:
	cc -std1 -O2 \
		-I/usr/dt/include \
		-I/usr/X11R6/include \
		-L/usr/dt/lib \
		-L/usr/X11R6/lib \
		-L/usr/shlib \
		-rpath /usr/dt/lib:/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -ldnet_stub

# Tru64 UNIX with Motif 1.2 (older systems)
tru64-motif12:
	cc -std1 -O2 \
		-I/usr/include/Xm \
		-I/usr/include/X11 \
		-L/usr/lib \
		-L/usr/shlib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -ldnet_stub -lc

# Tru64 with GCC
tru64-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/X11R6/include \
		-L/usr/X11R6/lib \
		-Wl,-rpath,/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Digital UNIX 4.0 (older name for Tru64)
digitalu:
	cc -std1 -O2 \
		-I/usr/X11R6/include \
		-L/usr/X11R6/lib \
		-L/usr/shlib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -ldnet_stub

# Digital UNIX 4.0 with GCC
digitalu-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/X11R6/include \
		-L/usr/X11R6/lib \
		-L/usr/shlib \
		-Wl,-rpath,/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# OSF/1 3.x (oldest name)
osf1:
	cc -std1 -O \
		-I/usr/include/X11 \
		-I/usr/include/Xm \
		-L/usr/lib \
		-L/usr/shlib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -ldnet_stub

# OSF/1 3.x with GCC
osf1-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include/X11 \
		-I/usr/include/Xm \
		-L/usr/lib \
		-L/usr/shlib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Generic/Old UNIX systems
# =============================================================================

# Generic Motif 1.2 systems
motif12:
	$(CC) -ansi -Wall -O2 \
		-I/usr/X11R6/include \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lXext

# Generic SVR4 system with vendor compiler
svr4:
	cc -O -Xa \
		-I/usr/X/include \
		-L/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lm -lsocket -lnsl -lgen

# Generic SVR4 system with GCC
svr4-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/X/include \
		-I/usr/include \
		-L/usr/X/lib \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lXext -lm -lsocket -lnsl -lgen

# Generic SVR3 system with vendor compiler (older)
svr3:
	cc -O \
		-I/usr/include/X11 \
		-L/usr/lib/X11 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lPW

# Generic SVR3 system with GCC
svr3-gcc:
	gcc -ansi -Wall -O \
		-I/usr/include/X11 \
		-I/usr/include \
		-L/usr/lib/X11 \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lPW

# =============================================================================
# macOS / Darwin with XQuartz and OpenMotif
# =============================================================================

macos:
	gcc -ansi -Wall -O2 \
		-I/opt/X11/include \
		-I/usr/local/include \
		-L/opt/X11/lib \
		-L/usr/local/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

macos-homebrew:
	gcc -ansi -Wall -O2 \
		-I/opt/homebrew/include \
		-I/opt/X11/include \
		-L/opt/homebrew/lib \
		-L/opt/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Cygwin / MSYS2 on Windows
# =============================================================================

cygwin:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

msys2:
	gcc -ansi -Wall -O2 \
		-I/mingw64/include \
		-L/mingw64/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Generic GCC builds for various versions
# =============================================================================

# Generic GCC build - auto-detect most settings
generic-gcc:
	gcc -ansi -Wall -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-I/usr/X11/include \
		-I/usr/local/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-L/usr/X11/lib \
		-L/usr/local/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lXext -lm

# GCC old versions - more conservative flags
gcc-old:
	gcc -ansi -pedantic -O \
		-I/usr/include \
		-I/usr/X11R6/include \
		-I/usr/X11/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lXext -lm

# GCC 2.7.x specific (circa 1995-1996)
gcc-27:
	gcc -ansi -pedantic -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# GCC 2.8.x specific (circa 1997-1998)
gcc-28:
	gcc -ansi -pedantic -Wall -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lXext -lm

# GCC 2.95.x specific (circa 1999-2001)
gcc-295:
	gcc -ansi -pedantic -Wall -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-I/usr/local/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-L/usr/local/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lXext -lm

# GCC 3.x specific (circa 2001-2005)
gcc-3:
	gcc -ansi -pedantic -Wall -W -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# GCC 4.x and newer
gcc-4:
	gcc -ansi -pedantic -Wall -Wextra -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-I/usr/include/X11 \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# EGCS 1.0.x / 1.1.x (became GCC 2.95)
egcs:
	egcs -ansi -pedantic -Wall -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lXext -lm

# =============================================================================
# Other Compilers
# =============================================================================

# Clang
clang:
	clang -ansi -Wall -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

clang-linux:
	clang -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

clang-freebsd:
	clang -ansi -Wall -O2 \
		-I/usr/local/include \
		-I/usr/X11R6/include \
		-L/usr/local/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Intel C Compiler
icc:
	icc -ansi -Wall -O2 \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

icc-linux:
	icc -std=c89 -Wall -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Portland Group Compiler
pgcc:
	pgcc -c89 -O2 \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Oracle Developer Studio (formerly Sun Studio)
sunstudio:
	cc -Xa -O \
		-I/usr/include \
		-I/usr/X11/include \
		-L/usr/lib \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

sunstudio-linux:
	suncc -Xa -O \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Cross-compilation targets
# =============================================================================

cross-arm:
	arm-linux-gnueabihf-gcc -ansi -Wall -O2 \
		-I/usr/arm-linux-gnueabihf/include \
		-L/usr/arm-linux-gnueabihf/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

cross-ppc:
	powerpc-linux-gnu-gcc -ansi -Wall -O2 \
		-I/usr/powerpc-linux-gnu/include \
		-L/usr/powerpc-linux-gnu/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

cross-mips:
	mips-linux-gnu-gcc -ansi -Wall -O2 \
		-I/usr/mips-linux-gnu/include \
		-L/usr/mips-linux-gnu/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Special build targets
# =============================================================================

# Static linking (where supported)
static:
	gcc -static -ansi -Wall -O2 \
		$(INCLUDES) \
		$(LIBDIRS) \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# 32-bit build on 64-bit system
32bit:
	gcc -m32 -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib32 -L/usr/lib/i386-linux-gnu \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# 64-bit build explicitly
64bit:
	gcc -m64 -ansi -Wall -O2 \
		-I/usr/include \
		-L/usr/lib64 -L/usr/lib/x86_64-linux-gnu \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Profiling build
profile:
	gcc -pg -ansi -Wall -O2 \
		$(INCLUDES) \
		$(LIBDIRS) \
		-o $(TARGET) $(SOURCES) \
		$(LIBS)

# Debugging with GDB support
gdb:
	gcc -g -O0 -ansi -Wall \
		$(INCLUDES) \
		$(LIBDIRS) \
		-o $(TARGET) $(SOURCES) \
		$(LIBS)

# Minimal build - bare essentials
minimal:
	gcc -ansi \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# K&R C compatibility mode (very old compilers)
knr:
	gcc -traditional-cpp -O \
		-I/usr/include \
		-I/usr/X11R6/include \
		-L/usr/lib \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Analysis and testing targets
# =============================================================================

analyze:
	cppcheck --enable=all --std=c89 $(SOURCES)

valgrind:
	gcc -g -O0 -ansi -Wall \
		$(INCLUDES) \
		$(LIBDIRS) \
		-o $(TARGET) $(SOURCES) \
		$(LIBS)

sanitize:
	gcc -g -O1 -ansi -Wall \
		-fsanitize=address -fsanitize=undefined \
		$(INCLUDES) \
		$(LIBDIRS) \
		-o $(TARGET) $(SOURCES) \
		$(LIBS)

coverage:
	gcc -g -O0 --coverage -ansi -Wall \
		$(INCLUDES) \
		$(LIBDIRS) \
		-o $(TARGET) $(SOURCES) \
		$(LIBS)

# =============================================================================
# Rare/Historical UNIX Systems
# =============================================================================

# NeXTSTEP / OPENSTEP (with X11 compatibility layer)
nextstep:
	cc -ansi -O \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Ultrix (DEC MIPS/VAX)
ultrix:
	cc -O \
		-I/usr/include/X11 \
		-I/usr/include/Xm \
		-L/usr/lib/X11 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# SunOS 4.x (pre-Solaris)
sunos4:
	gcc -ansi -O2 \
		-I/usr/openwin/include \
		-L/usr/openwin/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# A/UX (Apple UNIX)
aux:
	cc -O \
		-I/usr/include/X11 \
		-L/usr/lib/X11 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Xenix (older SCO/Microsoft UNIX)
xenix:
	cc -O \
		-I/usr/include/X11 \
		-L/usr/lib/X11 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lx

# Interactive UNIX (Sun)
interactive:
	cc -O -Xa \
		-I/usr/X/include \
		-L/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lsocket -lnsl

# Sequent DYNIX/ptx
dynix:
	cc -O \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lsocket -lnsl

# Data General DG/UX
dgux:
	gcc -ansi -O2 \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# NCR UNIX (SVR4-based)
ncr:
	cc -O -Xa \
		-I/usr/X/include \
		-L/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lm -lsocket -lnsl -lgen

# Pyramid UNIX
pyramid:
	cc -O \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Motorola System V/88 (m88k)
sysv88:
	cc -O \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lsocket -lnsl

# Bull DPX/20 (AIX-based)
bull:
	cc -O \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Siemens Nixdorf SINIX
sinix:
	cc -O -Xa \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm -lsocket -lnsl

# Unisys UNIX (SVR4)
unisys:
	cc -O -Xa \
		-I/usr/X/include \
		-L/usr/X/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lSM -lICE -lm -lsocket -lnsl -lgen

# Concurrent/Masscomp RTU
concurrent:
	cc -O \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Stratus VOS POSIX
stratus:
	cc -O \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# Tandem NonStop UNIX
tandem:
	cc -O -Wextensions \
		-I/usr/include \
		-L/usr/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Embedded / Special Purpose
# =============================================================================

# QNX 4.x with Photon (if X11 compatibility available)
qnx4:
	cc -O \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# QNX 6.x / Neutrino
qnx6:
	qcc -Vgcc_ntox86 -O2 \
		-I/usr/X11R6/include \
		-L/usr/X11R6/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# LynxOS
lynxos:
	gcc -ansi -O2 \
		-I/usr/X11/include \
		-L/usr/X11/lib \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# VxWorks (if X11 support available)
vxworks:
	ccppc -ansi -O2 \
		-I$(WIND_BASE)/target/h \
		-I$(WIND_BASE)/target/h/X11 \
		-o $(TARGET) $(SOURCES) \
		-lXm -lXt -lX11 -lm

# =============================================================================
# Help target
# =============================================================================

help:
	@echo "Paint Program Makefile"
	@echo "======================"
	@echo ""
	@echo "Usage: make <target>"
	@echo ""
	@echo "Main targets:"
	@echo "  all            - Build with default settings (GCC)"
	@echo "  clean          - Remove object files and binary"
	@echo "  install        - Install to /usr/local/bin"
	@echo "  debug          - Build with debugging symbols"
	@echo "  help           - Show this help message"
	@echo ""
	@echo "Linux targets:"
	@echo "  linux          - Linux with OpenMotif"
	@echo "  linux-lesstif  - Linux with LessTif"
	@echo "  linux-old      - Older Linux (X11R5/Motif 1.2)"
	@echo "  linux64        - Linux 64-bit"
	@echo "  linux-local    - Linux with /usr/local Motif"
	@echo "  linux-debian   - Debian/Ubuntu paths"
	@echo "  linux-redhat   - Red Hat/CentOS/Fedora"
	@echo "  linux-slackware - Slackware Linux"
	@echo ""
	@echo "BSD targets:"
	@echo "  freebsd        - FreeBSD"
	@echo "  freebsd-ports  - FreeBSD with ports"
	@echo "  netbsd         - NetBSD"
	@echo "  openbsd        - OpenBSD"
	@echo "  dragonfly      - DragonFly BSD"
	@echo ""
	@echo "Solaris/illumos targets:"
	@echo "  solaris        - Solaris with Sun cc"
	@echo "  solaris-gcc    - Solaris with GCC"
	@echo "  solaris-omf    - Solaris 10/11 OpenMotif"
	@echo "  solaris7       - Solaris 2.6/7"
	@echo "  solaris8       - Solaris 8/9"
	@echo "  solaris10-sparc64 - Solaris 10 SPARC 64-bit"
	@echo "  solaris10-amd64   - Solaris 10 x86 64-bit"
	@echo "  solaris-gcc-sparc64 - Solaris GCC SPARC 64-bit"
	@echo "  solaris-gcc-amd64   - Solaris GCC x86 64-bit"
	@echo "  openindiana    - OpenIndiana/illumos"
	@echo ""
	@echo "HP-UX targets:"
	@echo "  hpux           - HP-UX with HP cc (Motif 1.2)"
	@echo "  hpux9          - HP-UX 9.x"
	@echo "  hpux10         - HP-UX 10.x"
	@echo "  hpux11         - HP-UX 11.x (Motif 2.1)"
	@echo "  hpux11-64      - HP-UX 11.x 64-bit PA-RISC"
	@echo "  hpux-gcc       - HP-UX with GCC"
	@echo "  hpux10-gcc     - HP-UX 10.x with GCC"
	@echo "  hpux11-gcc     - HP-UX 11.x with GCC"
	@echo "  hpux11-gcc-64  - HP-UX 11.x GCC 64-bit"
	@echo "  hpux11i-ia64   - HP-UX 11i Itanium 64-bit"
	@echo "  hpux11i-ia64-gcc   - HP-UX 11i Itanium GCC"
	@echo "  hpux11i-ia64-32    - HP-UX 11i Itanium 32-bit"
	@echo "  hpux11i-ia64-gcc-32 - HP-UX 11i Itanium GCC 32-bit"
	@echo ""
	@echo "AIX targets:"
	@echo "  aix            - AIX with XL C"
	@echo "  aix4           - AIX 4.x"
	@echo "  aix5           - AIX 5.x"
	@echo "  aix-64         - AIX 64-bit"
	@echo "  aix-gcc        - AIX with GCC"
	@echo "  aix-gcc-64     - AIX GCC 64-bit"
	@echo "  aix-threads    - AIX with threads"
	@echo ""
	@echo "IRIX targets:"
	@echo "  irix           - SGI IRIX (o32 ABI)"
	@echo "  irix-n32       - SGI IRIX (n32 ABI)"
	@echo "  irix-64        - SGI IRIX (64-bit)"
	@echo "  irix-gcc       - SGI IRIX with GCC"
	@echo "  irix-gcc-n32   - SGI IRIX with GCC (n32 ABI)"
	@echo ""
	@echo "SCO OpenServer targets:"
	@echo "  sco-osr5       - SCO OpenServer 5.x"
	@echo "  sco-osr5-gcc   - SCO OpenServer 5.x GCC"
	@echo "  sco-osr6       - SCO OpenServer 6.x"
	@echo "  sco-osr6-gcc   - SCO OpenServer 6.x GCC"
	@echo ""
	@echo "SCO UnixWare targets:"
	@echo "  unixware2      - UnixWare 2.x"
	@echo "  unixware2-gcc  - UnixWare 2.x GCC"
	@echo "  unixware       - UnixWare 7.x"
	@echo "  unixware7      - UnixWare 7.x (alt)"
	@echo "  unixware-gcc   - UnixWare 7.x GCC"
	@echo "  unixware7-x11r5 - UnixWare 7.x X11R5"
	@echo "  unixware7-x11r6 - UnixWare 7.x X11R6"
	@echo "  unixware-alt   - UnixWare alternate lib paths"
	@echo ""
	@echo "Tru64/Digital UNIX targets:"
	@echo "  tru64          - Tru64 UNIX"
	@echo "  tru64-cde      - Tru64 with CDE"
	@echo "  tru64-motif12  - Tru64 Motif 1.2"
	@echo "  tru64-gcc      - Tru64 GCC"
	@echo "  digitalu       - Digital UNIX 4.0"
	@echo "  digitalu-gcc   - Digital UNIX GCC"
	@echo "  osf1           - OSF/1 3.x"
	@echo "  osf1-gcc       - OSF/1 GCC"
	@echo ""
	@echo "macOS/Darwin targets:"
	@echo "  macos          - macOS with XQuartz"
	@echo "  macos-homebrew - macOS with Homebrew"
	@echo ""
	@echo "Windows compatibility:"
	@echo "  cygwin         - Cygwin"
	@echo "  msys2          - MSYS2"
	@echo ""
	@echo "Generic targets:"
	@echo "  motif12        - Generic Motif 1.2"
	@echo "  svr4           - Generic SVR4"
	@echo "  svr4-gcc       - SVR4 with GCC"
	@echo "  svr3           - Generic SVR3"
	@echo "  svr3-gcc       - SVR3 with GCC"
	@echo ""
	@echo "GCC version targets:"
	@echo "  generic-gcc    - Auto-detect"
	@echo "  gcc-old        - Old GCC"
	@echo "  gcc-27         - GCC 2.7.x"
	@echo "  gcc-28         - GCC 2.8.x"
	@echo "  gcc-295        - GCC 2.95.x"
	@echo "  gcc-3          - GCC 3.x"
	@echo "  gcc-4          - GCC 4.x+"
	@echo "  egcs           - EGCS"
	@echo ""
	@echo "Other compilers:"
	@echo "  clang          - Clang"
	@echo "  clang-linux    - Clang on Linux"
	@echo "  clang-freebsd  - Clang on FreeBSD"
	@echo "  icc            - Intel C Compiler"
	@echo "  icc-linux      - ICC on Linux"
	@echo "  pgcc           - Portland Group"
	@echo "  sunstudio      - Oracle Developer Studio"
	@echo ""
	@echo "Special targets:"
	@echo "  static         - Static linking"
	@echo "  32bit          - 32-bit build"
	@echo "  64bit          - 64-bit build"
	@echo "  profile        - Profiling"
	@echo "  gdb            - GDB debugging"
	@echo "  minimal        - Minimal build"
	@echo "  valgrind       - Valgrind build"
	@echo "  sanitize       - Sanitizer build"
	@echo "  coverage       - Coverage build"
	@echo "  analyze        - Static analysis"
	@echo ""
	@echo "Historical UNIX:"
	@echo "  nextstep       - NeXTSTEP/OPENSTEP"
	@echo "  ultrix         - DEC Ultrix"
	@echo "  sunos4         - SunOS 4.x"
	@echo "  aux            - Apple A/UX"
	@echo "  xenix          - Xenix"
	@echo "  interactive    - Interactive UNIX"
	@echo "  dynix          - Sequent DYNIX/ptx"
	@echo "  dgux           - Data General DG/UX"
	@echo "  ncr            - NCR UNIX"
	@echo "  pyramid        - Pyramid UNIX"
	@echo "  sysv88         - Motorola System V/88"
	@echo "  bull           - Bull DPX/20"
	@echo "  sinix          - Siemens SINIX"
	@echo "  unisys         - Unisys UNIX"
	@echo "  concurrent     - Concurrent RTU"
	@echo "  stratus        - Stratus VOS"
	@echo "  tandem         - Tandem NonStop"
	@echo ""
	@echo "Embedded/Special:"
	@echo "  qnx4           - QNX 4.x"
	@echo "  qnx6           - QNX 6.x/Neutrino"
	@echo "  lynxos         - LynxOS"
	@echo "  vxworks        - VxWorks"
	@echo ""
	@echo "Cross-compilation:"
	@echo "  cross-arm      - ARM Linux"
	@echo "  cross-ppc      - PowerPC Linux"
	@echo "  cross-mips     - MIPS Linux"
	@echo ""