#pragma once

#if defined(_MSC_VER)
#define PBB_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define PBB_NOINLINE __attribute__((noinline))
#else
#define PBB_NOINLINE
#endif

#ifdef _MSC_VER
#define PBB_ATTR_DESTRUCTOR
// TODO: On microsoft one needs to handle load/unload of DLL's
#else
//#define PBB_ATTR_DESTRUCTOR
#define PBB_ATTR_DESTRUCTOR __attribute__((destructor(101)))
#endif
