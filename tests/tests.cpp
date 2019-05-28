#include <cassert>
#include "signals_unity_bridge.h"

using namespace std;

int main(int argc, char* argv[])
{
  auto pipeline = sub_create("MyPipeline");
  auto playbackSuccessful = sub_play(pipeline, "http://example.com/I_dont_exist.mpd");
  sub_destroy(pipeline);

  assert(!playbackSuccessful);

  return 0;
}

