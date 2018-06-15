MYDIR=$(call get-my-dir)

SUB_SRCS:=\
  $(LIB_MEDIA_SRCS)\
  $(LIB_MODULES_SRCS)\
  $(LIB_PIPELINE_SRCS)\
  $(LIB_UTILS_SRCS)\
  $(MYDIR)/plugin.cpp\

$(BIN)/signals-unity-bridge.so: $(SUB_SRCS:%=$(BIN)/%.o)
TARGETS+=$(BIN)/signals-unity-bridge.so

# Never compare the value of this variable to anything!
HOST:=$(shell $(CXX) -dumpmachine | sed "s/.*-//")

LOADER_SRCS:=\
	$(MYDIR)/loader.cpp\
	$(MYDIR)/dynlib_$(HOST).cpp\

include $(MYDIR)/$(HOST).mk

TARGETS+=$(BIN)/loader.exe
$(BIN)/loader.exe: $(LOADER_SRCS:%=$(BIN)/%.o)

$(BIN)/%.so:
	$(CXX) -static-libstdc++ -shared -o "$@" $^ \
		-Wl,--no-undefined \
		-Wl,--version-script=$(MYDIR)/plugin.version \
		$(LDFLAGS)
