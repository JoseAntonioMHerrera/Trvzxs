#pragma once
#define INCLUDE_ROUTE(route) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wunknown-pragmas\"") \
    _Pragma("GCC diagnostic ignored \"-Wpragmas\"") \
    _Pragma(STRINGIFY(include MACRO_TO_STRING(BASE_ROUTE) MACRO_TO_STRING(route))) \
    _Pragma("GCC diagnostic pop")

#define BASE_ROUTE /path/to/base/directory/
#define STRINGIFY(x) #x
#define MACRO_TO_STRING(x) STRINGIFY(x)
