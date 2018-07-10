// Signals-unity-bridge (SUB) interface

#pragma once

#define EXPORT __attribute__((visibility("default")))

extern "C" {

  // url: an http URL to a dash manifest (".mpd" file)
  // returns: an opaque player handle, or NULL on failure
	EXPORT void* sub_play(char const* url);

  // handle: a handle returned by 'sub_play'
	EXPORT void sub_stop(void* handle);
}

