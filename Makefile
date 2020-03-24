BIN?=bin
SRC?=signals/src
EXTRA?=signals/sysroot

CFLAGS+=-std=c++11 -fPIC

ifeq ($(DEBUG), 1)
  CFLAGS+=-g3
  LDFLAGS+=-g
else
  CFLAGS+=-O3
endif

ifeq ($(DEBUG), 0)
  # disable all warnings in release mode:
  # the code must always build, especially old versions with recent compilers
  CFLAGS+=-w -DNDEBUG
  LDFLAGS+=-Xlinker -s
endif

#------------------------------------------------------------------------------

include signals/Makefile
CFLAGS+=-Isrc

#------------------------------------------------------------------------------

include src/project.mk

#------------------------------------------------------------------------------

$(BIN)/signals_unity_bridge_version.mk:
	$(SRC)/../scripts/version.sh > $(BIN)/signals_unity_bridge_version.h
	@echo "" > "$@"
CFLAGS+=-I$(BIN)
ifneq ($(MAKECMDGOALS),clean)
include $(BIN)/signals_unity_bridge_version.mk
endif

targets: $(TARGETS)

