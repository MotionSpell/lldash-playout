BIN?=bin
SRC?=signals/src
EXTRA?=signals/sysroot

CFLAGS+=-fPIC

SIGNALS_HAS_X11?=0
#------------------------------------------------------------------------------

include signals/Makefile
CFLAGS+=-Isrc

#------------------------------------------------------------------------------

include src/project.mk

targets: $(TARGETS)
