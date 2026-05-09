// src/simdjson_impl.cpp
// simdjson implementation — ondemand API only (avoids DOM serializer AppleClang issue)

#define SIMDJSON_EXCEPTIONS 0
#define SIMDJSON_IMPLEMENTATION_ARM64 1
#include "simdjson.h"
