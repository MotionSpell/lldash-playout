#include <cassert>
#include <vector>
#include <future>
#include "lldash_play.h"

using namespace std;

int main(int argc, char* argv[])
{
  {
    auto pipeline = lldplay_create("MyPipeline");
    auto playbackSuccessful = lldplay_play(pipeline, "http://example.com/I_dont_exist.mpd");
    lldplay_destroy(pipeline);

    assert(!playbackSuccessful);
  }

  // parallel instantiations
  {
    vector<lldplay_handle*> pipelines;

    for(int i=0;i < 2;++i)
      pipelines.push_back(lldplay_create("MyPipeline"));

    for(auto pipeline : pipelines)
    {
      auto play = [pipeline]()
      {
        lldplay_play(pipeline, "http://example.com/I_dont_exist.mpd");
      };

      std::async(std::launch::async, play);
    }

    for(auto pipeline : pipelines)
      lldplay_destroy(pipeline);
  }

  return 0;
}
