#include "signals_unity_bridge.h"

#include <cstdio>
#include "lib_pipeline/pipeline.hpp"

// modules
#include "lib_media/demux/dash_demux.hpp"
#include "lib_media/demux/libav_demux.hpp"
#include "lib_media/in/mpeg_dash_input.hpp"
#include "lib_media/out/null.hpp"
#include "lib_media/decode/decoder.hpp"

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
				return pipeline.addModule<Demux::LibavDemux>(url);
			}
		};

		auto demuxer = createDemuxer(url);

		for (int k = 0; k < (int)demuxer->getNumOutputs(); ++k) {
			auto metadata = safe_cast<const MetadataPkt>(demuxer->getOutput(k)->getMetadata());
			if (!metadata || metadata->isSubtitle()/*only render audio and video*/) {
				Log::msg(Debug, "Ignoring stream #%s", k);
				continue;
			}

			auto decode = pipeline.addModule<Decode::Decoder>(metadata->getStreamType());
			pipeline.connect(demuxer, k, decode, 0);

			auto render = pipeline.addModule<Out::Null>();
			pipeline.connect(decode, 0, render, 0);
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
