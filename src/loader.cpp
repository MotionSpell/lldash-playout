#include <cstdio>
#include <cassert>
#include <dlfcn.h>

typedef void (*PlayFunction)(char const* url);

int main(int argc, char* argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Usage: %s <my_library>\n", argv[0]);
		return 1;
	}

	auto handle = dlopen(argv[1], RTLD_NOW);
	if(!handle) {
		fprintf(stderr, "can't load '%s': \n", argv[1]);
		return 1;
	}

	auto play = (PlayFunction)dlsym(handle, "play");
	if(!play) {
		fprintf(stderr, "'play' function not found\n");
		return 1;
	}

	play("http://this_is_my_url");

	dlclose(handle);
	return 0;
}
