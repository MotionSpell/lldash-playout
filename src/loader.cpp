// Non-interactive loader for signals-unity-bridge.so.
// Used for automated testing.
#include <cstdio>
#include <stdexcept>
#include "dynlib.h"

// API entry points. We don't call these directly,
// as the implemtations live inside the dynamic library.
#include "signals_unity_bridge.h"

using namespace std;

#define IMPORT(name) ((decltype(name)*)lib->getSymbol(# name))

void safeMain(int argc, char* argv[])
{
  if(argc != 3)
  {
    throw runtime_error("Usage: loader.exe <my_library> <my_url>");
  }

  auto libName = argv[1];
  auto url = argv[2];

  {
    auto lib = loadLibrary(libName);
    auto func_sub_create = IMPORT(sub_create);
    auto func_sub_destroy = IMPORT(sub_destroy);
    auto func_sub_play = IMPORT(sub_play);

    auto pipeline = func_sub_create(nullptr);
    func_sub_play(pipeline, url);
    func_sub_destroy(pipeline);
  }
}

int main(int argc, char* argv[])
{
  try
  {
    safeMain(argc, argv);
    return 0;
  }
  catch(exception const& e)
  {
    fprintf(stderr, "Fatal: %s\n", e.what());
    return 1;
  }
}

