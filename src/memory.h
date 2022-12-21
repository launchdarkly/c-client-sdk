#pragma once

extern void *(*LDi_customAlloc)(const size_t bytes);
extern void (*LDi_customFree)(void *const buffer);
extern char *(*LDi_customStrDup)(const char *const string);
extern void *(*LDi_customRealloc)(void *const buffer, const size_t bytes);
extern void *(*LDi_customCalloc)(const size_t nmemb, const size_t size);
extern char *(*LDi_customStrNDup)(const char *const str, const size_t n);
