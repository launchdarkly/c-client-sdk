/*!
 * @file export.h
 * @brief Public. Configuration of exported symbols.
 */

#pragma once

/** @brief Used to ensure only intended symbols are exported in the binaries */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
#define LD_EXPORT(x) x
#else
#ifdef _WIN32
#define LD_EXPORT(x) __declspec(dllexport) x
#else
#define LD_EXPORT(x) __attribute__((visibility("default"))) x
#endif
#endif
