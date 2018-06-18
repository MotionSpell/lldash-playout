#include <cstdio>
#include <stdexcept>
#include "dynlib.h"

using namespace std;

typedef void* (*PlayFunction)(char const* url);
typedef void (*StopFunction)(void*);

void safeMain(int argc, char* argv[]) {
	if(argc != 3) {
		throw runtime_error("Usage: loader.exe <my_library> <my_url>");
	}

	auto libName = argv[1];
	auto url = argv[2];

	{
		auto lib = loadLibrary(libName);
		auto play = (PlayFunction)lib->getSymbol("sub_play");
		auto stop = (StopFunction)lib->getSymbol("sub_stop");
		auto handle = play(url);
		stop(handle);
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

