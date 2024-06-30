#ifndef STRING_HELPER_MACROS_H
#define STRING_HELPER_MACROS_H

#define ARE_STRINGS_EQUAL(str1, str2) ({ \
    const char *_s1 = (str1); \
    const char *_s2 = (str2); \
    int _result = 1; \
    while (*_s1 != '\0' && *_s2 != '\0') { \
        if (*_s1 != *_s2) { \
            _result = 0; \
            break; \
        } \
        _s1++; \
        _s2++; \
    } \
    if (*_s1 != *_s2) { \
        _result = 0; \
    } \
    _result; \
})

#endif
