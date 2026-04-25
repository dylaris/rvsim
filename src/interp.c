#if defined(PURE)
    #include "interp/interp_pure.c"
#elif defined(DEBUG)
    #include "interp/interp_debug.c"
#elif defined(DBCACHE)
    #include "interp/interp_dbcache.c"
#elif defined(TBCACHE)
    #include "interp/interp_tbcache.c"
#endif
