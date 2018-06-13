#include <cstdio>

#define EXPORT __attribute__((visibility("default")))

extern "C" {
	EXPORT void play(char const* url) {
		printf("playing '%s'\n", url);
	}
}

