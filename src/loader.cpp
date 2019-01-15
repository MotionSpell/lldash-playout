#include <cstdio>
#include <stdexcept>
#include "dynlib.h"

// API entry points. We don't call these directly,
// as the implemtations live inside the dynamic library.
#include "signals_unity_bridge.h"

using namespace std;

#define IMPORT(name) ((decltype(name)*)lib->getSymbol(#name))

void safeMain(int argc, char* argv[]) {
	if(argc != 3) {
		throw runtime_error("Usage: loader.exe <my_library> <my_url>");
	}

	auto libName = argv[1];
	auto url = argv[2];

	{
		auto lib = loadLibrary(libName);
		auto func_gub_pipeline_create = IMPORT(gub_pipeline_create);
		auto func_gub_pipeline_destroy = IMPORT(gub_pipeline_destroy);
		auto func_gub_pipeline_setup_decoding = IMPORT(gub_pipeline_setup_decoding);
		auto func_gub_pipeline_play = IMPORT(gub_pipeline_play);

		GUBPipelineVars vars {};
		vars.uri = url;

		auto pipeline = func_gub_pipeline_create("myPipeline", nullptr, nullptr, nullptr, nullptr);
		func_gub_pipeline_setup_decoding(pipeline, &vars);
		func_gub_pipeline_play(pipeline);
		func_gub_pipeline_destroy(pipeline);
	}
}

int main(int argc, char* argv[]) {
	try {
		safeMain(argc, argv);
		return 0;
	} catch(exception const& e) {
		fprintf(stderr, "Fatal: %s\n", e.what());
		return 1;
	}
}

