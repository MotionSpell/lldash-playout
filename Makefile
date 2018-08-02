BIN?=bin
SRC?=signals/src
EXTRA?=signals/sysroot

CFLAGS+=-fPIC

#------------------------------------------------------------------------------

include signals/Makefile
CFLAGS+=-Isrc

#------------------------------------------------------------------------------

include src/project.mk

targets: $(TARGETS)
