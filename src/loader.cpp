#include <cstdio>
#include <cassert>
#include <dlfcn.h>

typedef void (*PlayFunction)(char const* url);

int main(int argc, char* argv[]) {
	if(argc != 3) {
		fprintf(stderr, "Usage: %s <my_library> <my_url>\n", argv[0]);
		return 1;
	}

	auto lib = argv[1];
	auto url = argv[2];

	auto handle = dlopen(lib, RTLD_NOW);
	if(!handle) {
		fprintf(stderr, "can't load '%s' (%s)\n", argv[1], dlerror());
		return 1;
	}

	auto play = (PlayFunction)dlsym(handle, "play");
	if(!play) {
		fprintf(stderr, "'play' function not found\n");
		return 1;
	}

	play(url);

	dlclose(handle);

	printf("Input file successfully processed.\n");

	return 0;
}
