#include "signals_unity_bridge.h"

#include <cstdio>
#include "lib_pipeline/pipeline.hpp"

// modules
#include "lib_media/demux/dash_demux.hpp"
#include "lib_media/demux/libav_demux.hpp"
#include "lib_media/in/mpeg_dash_input.hpp"
#include "lib_media/out/null.hpp"

using namespace Modules;
using namespace Pipelines;
using namespace Demux;
using namespace std;

static
bool startsWith(string s, string prefix) {
	return s.substr(0, prefix.size()) == prefix;
}

void* sub_play(char const* url) {
	try {
		auto pipelinePtr = make_unique<Pipeline>();
		auto& pipeline = *pipelinePtr;

		auto createDemuxer = [&](string url) {
			if(startsWith(url, "http://")) {
				return pipeline.addModule<DashDemuxer>(url);
			} else {
				DemuxConfig cfg;
				cfg.url = url;
				return pipeline.add("LibavDemux", &cfg);
			}
		};

		auto demuxer = createDemuxer(url);

		for (int k = 0; k < (int)demuxer->getNumOutputs(); ++k) {
			auto metadata = demuxer->getOutputMetadata(k);
			if (!metadata || metadata->isSubtitle()/*only render audio and video*/) {
				g_Log->log(Debug, format("Ignoring stream #%s", k).c_str());
				continue;
			}

			auto decode = pipeline.add("Decoder", (void*)(uintptr_t)metadata->type);
			pipeline.connect(GetOutputPin(demuxer, k), decode);

			auto render = pipeline.addModule<Out::Null>();
			pipeline.connect(decode, render);
		}

		pipeline.start();
		return pipelinePtr.release();
	} catch(runtime_error const& err) {
		fprintf(stderr, "cannot play: %s\n", err.what());
		return nullptr;
	}
}

void sub_stop(void* handle) {
	try {
		unique_ptr<Pipeline> pipeline(static_cast<Pipeline*>(handle));
		if(!pipeline)
			throw runtime_error("invalid handle");
		pipeline->waitForEndOfStream();
	} catch(runtime_error const& err) {
		fprintf(stderr, "cannot stop: %s\n", err.what());
	}
}
