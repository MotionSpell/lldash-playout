///////////////////////////////////////////////////////////////////////////////
// interface

#define EXPORT __attribute__((visibility("default")))

extern "C" {
	EXPORT void play(char const* url);
}

///////////////////////////////////////////////////////////////////////////////
// implementation

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

static
bool startsWith(std::string s, std::string prefix) {
	return s.substr(0, prefix.size()) == prefix;
}

void play(char const* url) {
	try {
		Pipeline pipeline;

		auto createDemuxer = [&](std::string url) {
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
		pipeline.waitForEndOfStream();
	} catch(std::runtime_error const& err) {
		fprintf(stderr, "cannot play: %s\n", err.what());
	}
}

