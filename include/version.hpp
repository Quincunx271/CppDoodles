/*
How you might do version macros for a library
*/

#define X_VERSION_MAJOR 1
#define X_VERSION_MINOR 2
#define X_VERSION_PATCH 3

#define X_MAKE_VERSION(M, m, p) (((M) << 24) | ((m) << 8) | ((p) << 0))
#define X_VERSION X_MAKE_VERSION(X_VERSION_MAJOR, X_VERSION_MINOR, X_VERSION_PATCH)

#if X_VERSION <= X_MAKE_VERSION(1, 2, 3)
int foo() { return -1; }
#endif

#define X_DETAIL_HAS_FEATURE_function_objects X_MAKE_VERSION(1, 1, 0)
#define X_HAS_FEATURE(...) (X_DETAIL_HAS_FEATURE_ ## __VA_ARGS__)

#if X_HAS_FEATURE(function_objects)
int bar() {
    return -3;
}
#endif
