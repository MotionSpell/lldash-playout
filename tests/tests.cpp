#include <cassert>
#include <vector>
#include <future>
#include "lldash_play.h"

using namespace std;

int main(int argc, char* argv[])
{
  {
    auto pipeline = sub_create("MyPipeline");
    auto playbackSuccessful = sub_play(pipeline, "http://example.com/I_dont_exist.mpd");
    sub_destroy(pipeline);

    assert(!playbackSuccessful);
  }

  // parallel instantiations
  {
    vector<sub_handle*> pipelines;

    for(int i=0;i < 2;++i)
      pipelines.push_back(sub_create("MyPipeline"));

    for(auto pipeline : pipelines)
    {
      auto play = [pipeline]()
      {
        sub_play(pipeline, "http://example.com/I_dont_exist.mpd");
      };

      std::async(std::launch::async, play);
    }

    for(auto pipeline : pipelines)
      sub_destroy(pipeline);
  }

  return 0;
}
