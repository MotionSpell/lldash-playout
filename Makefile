BIN?=bin
SRC?=signals/src
EXTRA?=signals/sysroot

CFLAGS+=-fPIC

PKGS+=sdl2
#------------------------------------------------------------------------------

include signals/Makefile
CFLAGS+=-Isrc

#------------------------------------------------------------------------------

include src/project.mk

targets: $(TARGETS)
