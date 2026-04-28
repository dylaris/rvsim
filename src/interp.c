#if defined(PURE)
    #include "interp/interp_pure.c"
#elif defined(DEBUG)
    #include "interp/interp_debug.c"
#elif defined(CACHE)
    #include "interp/interp_cache.c"
#endif
