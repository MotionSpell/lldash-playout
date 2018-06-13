MYDIR=$(call get-my-dir)

SUB_SRCS:=\
  $(LIB_MEDIA_SRCS)\
  $(LIB_MODULES_SRCS)\
  $(LIB_PIPELINE_SRCS)\
  $(LIB_UTILS_SRCS)\
  $(MYDIR)/plugin.cpp\

$(BIN)/signals-unity-bridge.so: $(SUB_SRCS:%=$(BIN)/%.o)
TARGETS+=$(BIN)/signals-unity-bridge.so

$(BIN)/%.so:
	$(CXX) -static-libstdc++ -shared -o "$@" $^ -Wl,--no-undefined $(LDFLAGS)
