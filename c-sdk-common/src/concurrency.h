#pragma once

#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif

#ifdef _WIN32
    #define THREAD_RETURN DWORD
    #define THREAD_RETURN_DEFAULT 0
    #define ld_thread_t HANDLE

    #define ld_mutex_t CRITICAL_SECTION
    #define ld_cond_t CONDITION_VARIABLE

    #ifdef LAUNCHDARKLY_MUTEX_ONLY
        #define ld_rwlock_t ld_mutex_t
        #define LD_RWLOCK_INIT LD_MUTEX_INIT
    #else
        #define ld_rwlock_t SRWLOCK
        #define LD_RWLOCK_INIT SRWLOCK_INIT
    #endif
#else
    #define THREAD_RETURN void *
    #define THREAD_RETURN_DEFAULT NULL
    #define ld_thread_t pthread_t

    #define ld_mutex_t pthread_mutex_t
    #define ld_cond_t pthread_cond_t

    #ifdef LAUNCHDARKLY_MUTEX_ONLY
        #define ld_rwlock_t ld_mutex_t
        #define LD_RWLOCK_INIT LD_MUTEX_INIT
    #else
        #define ld_rwlock_t pthread_rwlock_t
        #define LD_RWLOCK_INIT PTHREAD_RWLOCK_INITIALIZER
    #endif
#endif

typedef bool (*ld_mutex_unary_t)(ld_mutex_t *const mutex);

typedef bool (*ld_thread_join_t)(ld_thread_t *const thread);
typedef bool (*ld_thread_create_t)(ld_thread_t *const thread,
    THREAD_RETURN (*const routine)(void *), void *const argument);

typedef bool (*ld_rwlock_unary_t)(ld_rwlock_t *const lock);

typedef bool (*ld_cond_unary_t)(ld_cond_t *const cond);
typedef bool (*ld_cond_wait_t)(ld_cond_t *const cond, ld_mutex_t *const mutex,
    const int milliseconds);

extern ld_mutex_unary_t   LDi_mutex_init;
extern ld_mutex_unary_t   LDi_mutex_destroy;
extern ld_mutex_unary_t   LDi_mutex_lock;
extern ld_mutex_unary_t   LDi_mutex_unlock;

extern ld_thread_join_t   LDi_thread_join;
extern ld_thread_create_t LDi_thread_create;

extern ld_rwlock_unary_t  LDi_rwlock_init;
extern ld_rwlock_unary_t  LDi_rwlock_destroy;
extern ld_rwlock_unary_t  LDi_rwlock_rdlock;
extern ld_rwlock_unary_t  LDi_rwlock_wrlock;
extern ld_rwlock_unary_t  LDi_rwlock_rdunlock;
extern ld_rwlock_unary_t  LDi_rwlock_wrunlock;

extern ld_cond_unary_t    LDi_cond_init;
extern ld_cond_wait_t     LDi_cond_wait;
extern ld_cond_unary_t    LDi_cond_signal;
extern ld_cond_unary_t    LDi_cond_destroy;
