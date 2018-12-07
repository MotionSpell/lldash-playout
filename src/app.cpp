#include <cstdio>
#include <exception>
#include "dynlib.h"
#include "signals_unity_bridge.h"

using namespace std;

void onError(GUBPipeline* userData, char* message) {
	(void)userData;
	printf("Error: '%s'\n", message);
}

void safeMain(int argc, char* argv[]) {
	if(argc != 2) {
		throw runtime_error("Usage: app.exe <my_library>");
	}

	auto libName = argv[1];

	{
		auto lib = loadLibrary(libName);
		auto func_gub_pipeline_create = (decltype(gub_pipeline_create)*)lib->getSymbol("gub_pipeline_create");
		auto func_gub_pipeline_play = (decltype(gub_pipeline_play)*)lib->getSymbol("gub_pipeline_create");
		auto func_gub_pipeline_destroy = (decltype(gub_pipeline_destroy)*)lib->getSymbol("gub_pipeline_destroy");
		auto handle = func_gub_pipeline_create("name", nullptr, &onError, nullptr, nullptr);
		func_gub_pipeline_play(handle);
		func_gub_pipeline_destroy(handle);
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

