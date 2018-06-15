#include <cstdio>
#include <cassert>
#include "dynlib.h"

typedef void (*PlayFunction)(char const* url);

int main(int argc, char* argv[]) {
	if(argc != 3) {
		fprintf(stderr, "Usage: %s <my_library> <my_url>\n", argv[0]);
		return 1;
	}

	auto libName = argv[1];
	auto url = argv[2];

	{
		auto lib = loadLibrary(libName);
		auto play = (PlayFunction)lib->getSymbol("play");
		play(url);
	}

	printf("Input file successfully processed.\n");

	return 0;
}
