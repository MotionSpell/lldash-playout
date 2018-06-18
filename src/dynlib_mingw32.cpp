// this file is MS Windows specific
#include "dynlib.h"
#include <windows.h>
#include <stdexcept>
#include <string>

using namespace std;

struct DynLibGnu : DynLib {
	DynLibGnu(const char* name) : handle(LoadLibrary(name)) {
		if(!handle) {
			string msg = "can't load '";
			msg += name;
			throw runtime_error(msg);
		}
	}

	~DynLibGnu() {
		FreeLibrary(handle);
	}

	virtual void* getSymbol(const char* name) {
		auto func = GetProcAddress(handle, "play");
		if(!func) {
			string msg = "can't find symbol '";
			msg += name;
			msg += "'";
			throw runtime_error(msg);
		}

		return (void*)func;
	}

	HMODULE const handle;
};

unique_ptr<DynLib> loadLibrary(const char* name) {
	return make_unique<DynLibGnu>(name);
}

