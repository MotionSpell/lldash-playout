#include <cstdio>
#include <exception>
#include "dynlib.h"
#include "signals_unity_bridge.h"

using namespace std;

void onError(GUBPipeline* userData, char* message) {
	(void)userData;
	printf("Error: '%s'\n", message);
}

#define IMPORT(name) ((decltype(name)*)lib->getSymbol(#name))

void safeMain(int argc, char* argv[]) {
	if(argc != 2) {
		throw runtime_error("Usage: app.exe <my_library>");
	}

	auto libName = argv[1];

	{
		auto lib = loadLibrary(libName);
		auto func_gub_ref = IMPORT(gub_ref);
		auto func_gub_unref = IMPORT(gub_unref);
		auto func_gub_pipeline_create = IMPORT(gub_pipeline_create);
		auto func_gub_pipeline_play = IMPORT(gub_pipeline_play);
		auto func_gub_pipeline_setup_decoding = IMPORT(gub_pipeline_setup_decoding);
		auto func_gub_pipeline_destroy = IMPORT(gub_pipeline_destroy);

		func_gub_ref("TheApp");

		auto handle = func_gub_pipeline_create("name", nullptr, &onError, nullptr, nullptr);

		GUBPipelineVars vars {};
		vars.uri = "http://livesim.dashif.org/livesim/testpic_2s/Manifest.mpd";
		func_gub_pipeline_setup_decoding(handle, &vars);

		func_gub_pipeline_play(handle);

		{
			printf("Press return to stop\n");
			char dummy[16];
			fgets(dummy, 16, stdin);
		}

		func_gub_pipeline_destroy(handle);

		func_gub_unref();
	}

	printf("Input file successfully processed.\n");
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

