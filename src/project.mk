MYDIR=$(call get-my-dir)

# Never compare the value of this variable to anything!
HOST:=$(shell $(CXX) -dumpmachine | sed "s/.*-//")

include $(MYDIR)/$(HOST).mk

#------------------------------------------------------------------------------
SUB_SRCS:=\
  $(LIB_MEDIA_SRCS)\
  $(LIB_MODULES_SRCS)\
  $(LIB_PIPELINE_SRCS)\
  $(LIB_UTILS_SRCS)\
  $(MYDIR)/plugin.cpp\

$(BIN)/signals-unity-bridge.so: $(SUB_SRCS:%=$(BIN)/%.o)
TARGETS+=$(BIN)/signals-unity-bridge.so
$(BIN)/signals-unity-bridge.so: LDFLAGS+=$(shell pkg-config gl --libs)

#------------------------------------------------------------------------------
LOADER_SRCS:=\
	$(MYDIR)/loader.cpp\
	$(MYDIR)/dynlib_$(HOST).cpp\

TARGETS+=$(BIN)/loader.exe
$(BIN)/loader.exe: $(LOADER_SRCS:%=$(BIN)/%.o)

#------------------------------------------------------------------------------
APP_SRCS:=\
	$(MYDIR)/app.cpp\
	$(MYDIR)/dynlib_$(HOST).cpp\

# For the moment, don't build interactive test app in release builds:
# we can't link a proper Windows executable, as we're not using any GL loader.
ifeq ($(VERSION),)
TARGETS+=$(BIN)/app.exe
endif

$(BIN)/app.exe: $(APP_SRCS:%=$(BIN)/%.o)
$(BIN)/app.exe: CFLAGS+=$(shell pkg-config sdl2 gl --cflags)
$(BIN)/app.exe: LDFLAGS+=$(shell pkg-config sdl2 gl --libs)

#------------------------------------------------------------------------------
# Generic rules
#
$(BIN)/%.so:
	$(CXX) -static-libstdc++ -shared -o "$@" $^ \
		-Wl,--no-undefined \
		-Wl,--version-script=$(MYDIR)/plugin.version \
		$(LDFLAGS)
