#pragma once
#ifndef HMH_DEFINES_H
#define HMH_DEFINES_H

//math macros
#define M_EPSILON    0.001f
#define M_PI         3.14159265359f
#define M_2PI        6.28318530718f
#define M_E          2.71828182846f
#define M_SQRT_TWO   1.41421356237f
#define M_SQRT_THREE 1.73205080757f
#define RADIANS(x) (x * (M_PI / 180.f))
#define DEGREES(x) (x * (180.f / M_PI))

//number typedefs
typedef signed char    s8;
typedef signed short   s16;
typedef signed int     s32;
typedef signed long    s64;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef float          f32;
typedef double         f64;
typedef s32            b32;

//static defines
#define static_internal static
#define local_persist   static
#define global_variable static

#define forn(iterator,iteration_count) for(int iterator = 0; iterator < iteration_count; ++i)

// delay the operation until the end of scope
// https://stackoverflow.com/a/42060129
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

//size of c-style array
#define ArrayCount(_ARR) (sizeof(_ARR) / sizeof((_ARR)[0]))

//#define Swap(a,b) {} (void)0

#endif //HMH_DEFINES_H
