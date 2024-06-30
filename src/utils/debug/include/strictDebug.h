


#ifdef STRICT_DEBUG_MODE
#define ASSERT_VALID_POINTER(pointer) assert(pointer);
#else
#define ASSERT_VALID_POINTER(pointer) ((void)0);
#endif
