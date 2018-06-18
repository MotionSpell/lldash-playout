// this file is GNU/Linux specific
#include "dynlib.h"
#include <dlfcn.h>
#include <stdexcept>
#include <string>

using namespace std;

struct DynLibGnu : DynLib {
	DynLibGnu(const char* name) : handle(dlopen(name, RTLD_NOW)) {
		if(!handle) {
			string msg = "can't load '";
			msg += name;
			msg += "' (";
			msg += dlerror();
			msg += ")";
			throw runtime_error(msg);
		}
	}

	~DynLibGnu() {
		dlclose(handle);
	}

	virtual void* getSymbol(const char* name) {
		auto func = dlsym(handle, name);
		if(!func) {
			string msg = "can't find symbol '";
			msg += name;
			msg += "'";
			throw runtime_error(msg);
		}

		return func;
	}

	void* const handle;
};

unique_ptr<DynLib> loadLibrary(const char* name) {
	return make_unique<DynLibGnu>(name);
}

