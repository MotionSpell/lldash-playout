#pragma once

#include <memory>

struct DynLib
{
  virtual ~DynLib() = default;
  virtual void* getSymbol(const char* name) = 0;
};

std::unique_ptr<DynLib> loadLibrary(const char* name);

