#include <cstdio>

extern "C"
{
	void play(char const* url) {
		printf("playing '%s'\n", url);
	}
}

