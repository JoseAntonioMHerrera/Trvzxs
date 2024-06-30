#ifndef _H_COMPARATION_HELPER_MACROS
#define _H_COMPARATION_HELPER_MACROS

#define CLAMP(value, min, max) \
    ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))

#endif
