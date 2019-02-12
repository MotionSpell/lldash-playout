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

#include "IUnityInterface.h"
#include "IUnityGraphics.h"
void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);

struct Host : IUnityInterfaces, IUnityGraphics {};

static Host g_host;

static IUnityInterface* UNITY_INTERFACE_API GetInterfaceImpl(UnityInterfaceGUID guid)
{
  (void)guid;
  return &g_host;
}

static UnityGfxRenderer UNITY_INTERFACE_API GetRendererImpl()
{
  return kUnityGfxRendererNull;
}

void safeMain(int argc, char* argv[])
{
  g_host.GetInterface = &GetInterfaceImpl;
  g_host.GetRenderer = &GetRendererImpl;

  if(argc != 3)
  {
    throw runtime_error("Usage: loader.exe <my_library> <my_url>");
  }

  auto libName = argv[1];
  auto url = argv[2];

  {
    auto lib = loadLibrary(libName);
    auto func_UnityPluginLoad = IMPORT(UnityPluginLoad);
    auto func_sub_create = IMPORT(sub_create);
    auto func_sub_destroy = IMPORT(sub_destroy);
    auto func_sub_play = IMPORT(sub_play);

    func_UnityPluginLoad(&g_host);

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

