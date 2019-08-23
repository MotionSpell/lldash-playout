MYDIR=$(call get-my-dir)

# Never compare the value of this variable to anything!
HOST:=$(shell $(CXX) -dumpmachine | sed "s/.*-\([a-zA-Z]*\)[0-9.]*/\1/")

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

#------------------------------------------------------------------------------
LOADER_SRCS:=\
	$(MYDIR)/loader.cpp\
	$(MYDIR)/dynlib_$(HOST).cpp\

TARGETS+=$(BIN)/loader.exe
$(BIN)/loader.exe: $(LOADER_SRCS:%=$(BIN)/%.o)

#------------------------------------------------------------------------------
# Keep this one dead-easy to build, even without the makefile:
TARGETS+=$(BIN)/example.exe
$(BIN)/example.exe: $(MYDIR)/example.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -o "$@" $^ -L$(BIN) -lsignals-unity-bridge

#------------------------------------------------------------------------------
# Generic rules
#
$(BIN)/%.so:
	$(CXX) $(CFLAGS) -static-libstdc++ -shared -o "$@" $^ \
		-Wl,--no-undefined \
		-Wl,--version-script=$(MYDIR)/plugin.version \
		$(LDFLAGS)
