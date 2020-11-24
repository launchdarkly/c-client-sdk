#include <errno.h>

#include "assertion.h"
#include "utility.h"
#include "logging.h"
#include "concurrency.h"

static bool
LDi_thread_join_imp(ld_thread_t *const thread)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_thread_join start %p", (void *)thread);
    #endif

    LD_ASSERT(thread);

    #ifdef _WIN32
        if ((status =
            (WaitForSingleObject(*thread, INFINITE) == WAIT_OBJECT_0) != true))
        {
            LD_LOG(LD_LOG_CRITICAL, "WaitForSingleObject failed");
        }
    #else
        if ((status = pthread_join(*thread, NULL)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_join failed: %s",
                strerror(status));
        }
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_thread_join end %p", (void *)thread);
    #endif

    return status == 0;
}

static bool
LDi_thread_create_imp(ld_thread_t *const thread,
    THREAD_RETURN (*const routine)(void *), void *const argument)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_2(LD_LOG_TRACE, "LDi_thread_create start %p %p",
            (void *)thread, (void *)argument);
    #endif

    LD_ASSERT(thread);
    LD_ASSERT(routine);

    #ifdef _WIN32
        *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)routine,
            argument, 0, NULL);

        status = (*thread) == NULL;
    #else
        if ((status = pthread_create(thread, NULL, routine, argument)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_create failed: %s",
                strerror(status));
        }
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_2(LD_LOG_TRACE, "LDi_thread_create end %p %p",
            (void *)thread, (void *)argument);
    #endif

    return status == 0;
}

static bool
LDi_mutex_init_imp(ld_mutex_t *const mutex)
{
    int status;

    #ifndef _WIN32
        pthread_mutexattr_t attributes;
        int kind;
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_mutex_init start %p", (void *)mutex);
    #endif

    LD_ASSERT(mutex);

    #ifdef _WIN32
        InitializeCriticalSection(mutex);

        status = 0;
    #else
        if ((status = pthread_mutexattr_init(&attributes)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_mutexattr_init failed: %s",
                strerror(status));

            goto done;
        }

        #ifdef LAUNCHDARKLY_CONCURRENCY_UNSAFE
            kind = PTHREAD_MUTEX_NORMAL;
        #else
            kind = PTHREAD_MUTEX_ERRORCHECK;
        #endif

        if ((status = pthread_mutexattr_settype(&attributes, kind)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_mutexattr_settype failed: %s",
                strerror(status));

            goto done;
        }

        if ((status = pthread_mutex_init(mutex, &attributes)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_mutex_init failed: %s",
                strerror(status));

            goto done;
        }

        /* this should never fail */
        if ((status = pthread_mutexattr_destroy(&attributes)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_mutexattr_destroy failed: %s",
                strerror(status));

            goto done;
        }
    #endif

  done:
    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_mutex_init end %p", (void *)mutex);
    #endif

    return status == 0;
}

static bool
LDi_mutex_init_nl_imp(ld_mutex_t *const mutex)
{
    int status;

    #ifndef _WIN32
        pthread_mutexattr_t attributes;
        int kind;
    #endif

    #ifdef _WIN32
        InitializeCriticalSection(mutex);

        status = 0;
    #else
        if ((status = pthread_mutexattr_init(&attributes)) != 0) {
            goto done;
        }

        #ifdef LAUNCHDARKLY_CONCURRENCY_UNSAFE
            kind = PTHREAD_MUTEX_NORMAL;
        #else
            kind = PTHREAD_MUTEX_ERRORCHECK;
        #endif

        if ((status = pthread_mutexattr_settype(&attributes, kind)) != 0) {
            goto done;
        }

        if ((status = pthread_mutex_init(mutex, &attributes)) != 0) {
            goto done;
        }

        /* this should never fail */
        if ((status = pthread_mutexattr_destroy(&attributes)) != 0) {
            goto done;
        }
    #endif

  done:
    return status == 0;
}

static bool
LDi_mutex_destroy_imp(ld_mutex_t *const mutex)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_mutex_destroy start %p", (void *)mutex);
    #endif

    LD_ASSERT(mutex);

    #ifdef _WIN32
        DeleteCriticalSection(mutex);

        status = 0;
    #else
        if ((status = pthread_mutex_destroy(mutex)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_mutex_destroy failed: %s",
                strerror(status));
        }
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_mutex_destroy end %p", (void *)mutex);
    #endif

    return status == 0;
}

static bool
LDi_mutex_destroy_nl_imp(ld_mutex_t *const mutex)
{
    int status;

    #ifdef _WIN32
        DeleteCriticalSection(mutex);

        status = 0;
    #else
        status = pthread_mutex_destroy(mutex);
    #endif

    return status == 0;
}

static bool
LDi_mutex_lock_imp(ld_mutex_t *const mutex)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_mutex_lock start %p", (void *)mutex);
    #endif

    LD_ASSERT(mutex);

    #ifdef _WIN32
        EnterCriticalSection(mutex);

        status = 0;
    #else
        if ((status = pthread_mutex_lock(mutex)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_mutex_lock failed: %s",
                strerror(status));
        }
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_mutex_lock end %p", (void *)mutex);
    #endif

    return status == 0;
}

static bool
LDi_mutex_lock_nl_imp(ld_mutex_t *const mutex)
{
    int status;

    #ifdef _WIN32
        EnterCriticalSection(mutex);

        status = 0;
    #else
        status = pthread_mutex_lock(mutex);
    #endif

    return status == 0;
}

static bool
LDi_mutex_unlock_imp(ld_mutex_t *const mutex)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_mutex_unlock start %p", (void *)mutex);
    #endif

    LD_ASSERT(mutex);

    #ifdef _WIN32
        LeaveCriticalSection(mutex);

        status = 0;
    #else
        if ((status = pthread_mutex_unlock(mutex)) != 0) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_mutex_unlock failed: %s",
                strerror(status));
        }
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_mutex_unlock end %p", (void *)mutex);
    #endif

    return status == 0;
}

static bool
LDi_mutex_unlock_nl_imp(ld_mutex_t *const mutex)
{
    int status;

    #ifdef _WIN32
        LeaveCriticalSection(mutex);

        status = 0;
    #else
        status = pthread_mutex_unlock(mutex);
    #endif

    return status == 0;
}

static bool
LDi_rwlock_init_imp(ld_rwlock_t *const lock)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_init start %p", (void *)lock);
    #endif

    LD_ASSERT(lock);

    #ifdef LAUNCHDARKLY_MUTEX_ONLY
        status = LDi_mutex_init(lock) != true;
    #else
        #ifdef _WIN32
            InitializeSRWLock(lock);

            status = 0;
        #else
            if ((status = pthread_rwlock_init(lock, NULL)) != 0) {
                LD_LOG_1(LD_LOG_CRITICAL, "pthread_rwlock_init failed: %s",
                    strerror(status));
            }
        #endif
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_init end %p", (void *)lock);
    #endif

    return status == 0;
}

static bool
LDi_rwlock_destroy_imp(ld_rwlock_t *const lock)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_destroy start %p", (void *)lock);
    #endif

    LD_ASSERT(lock);

    #ifdef LAUNCHDARKLY_MUTEX_ONLY
        status = LDi_mutex_destroy(lock) != true;
    #else
        #ifdef _WIN32
            /* no cleanup procedure for windows */

            status = 0;
        #else
            if ((status = pthread_rwlock_destroy(lock)) != 0) {
                LD_LOG_1(LD_LOG_CRITICAL, "pthread_rwlock_destroy failed: %s",
                    strerror(status));
            }
        #endif
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_destroy end %p", (void *)lock);
    #endif

    return status == 0;
}

static bool
LDi_rwlock_rdlock_imp(ld_rwlock_t *const lock)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_rdlock start %p", (void *)lock);
    #endif

    LD_ASSERT(lock);

    #ifdef LAUNCHDARKLY_MUTEX_ONLY
        status = LDi_mutex_lock(lock) != true;
    #else
        #ifdef _WIN32
            AcquireSRWLockShared(lock);

            status = 0;
        #else
            if ((status = pthread_rwlock_rdlock(lock)) != 0) {
                LD_LOG_1(LD_LOG_CRITICAL, "pthread_rwlock_rdlock failed: %s",
                    strerror(status));
            }
        #endif
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_rdlock end %p", (void *)lock);
    #endif

    return status == 0;
}

static bool
LDi_rwlock_wrlock_imp(ld_rwlock_t *const lock)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_wrlock start %p", (void *)lock);
    #endif

    LD_ASSERT(lock);

    #ifdef LAUNCHDARKLY_MUTEX_ONLY
        status = LDi_mutex_lock(lock) != true;
    #else
        #ifdef _WIN32
            AcquireSRWLockExclusive(lock);

            status = 0;
        #else
            if ((status = pthread_rwlock_wrlock(lock)) != 0) {
                LD_LOG_1(LD_LOG_CRITICAL, "pthread_rwlock_wrlock failed: %s",
                    strerror(status));
            }
        #endif
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_wrlock end %p", (void *)lock);
    #endif

    return status == 0;
}

static bool
LDi_rwlock_rdunlock_imp(ld_rwlock_t *const lock)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_rdunlock start %p", (void *)lock);
    #endif

    LD_ASSERT(lock);

    #ifdef LAUNCHDARKLY_MUTEX_ONLY
        status = LDi_mutex_unlock(lock) != true;
    #else
        #ifdef _WIN32
            ReleaseSRWLockShared(lock);

            status = 0;
        #else
            if ((status = pthread_rwlock_unlock(lock)) != 0) {
                LD_LOG_1(LD_LOG_CRITICAL, "pthread_rwlock_unlock failed: %s",
                    strerror(status));
            }
        #endif
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_rdunlock end %p", (void *)lock);
    #endif

    return status == 0;
}

static bool
LDi_rwlock_wrunlock_imp(ld_rwlock_t *const lock)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_wrunlock start %p", (void *)lock);
    #endif

    LD_ASSERT(lock);

    #ifdef LAUNCHDARKLY_MUTEX_ONLY
        status = LDi_mutex_unlock(lock) != true;
    #else
        #ifdef _WIN32
            ReleaseSRWLockExclusive(lock);

            status = 0;
        #else
            if ((status = pthread_rwlock_unlock(lock)) != 0) {
                LD_LOG_1(LD_LOG_CRITICAL, "pthread_rwlock_unlock failed: %s",
                    strerror(status));
            }
        #endif
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_rwlock_wrunlock end %p", (void *)lock);
    #endif

    return status == 0;
}

static bool
LDi_cond_wait_imp(ld_cond_t *const cond, ld_mutex_t *const mutex,
    const int milliseconds)
{
    int status;

    #ifndef _WIN32
        struct timespec ts;
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_3(LD_LOG_TRACE, "LDi_cond_wait start %p %p %d",
            (void *)cond, (void *)mutex, milliseconds);
    #endif

    LD_ASSERT(cond);
    LD_ASSERT(mutex);

    #ifdef _WIN32
        status = SleepConditionVariableCS(cond, mutex, milliseconds);

        if (status == 0) {
            if (GetLastError() != ERROR_TIMEOUT) {
                status = 1;
            }
        } else {
            status = 0;
        }
    #else
        if ((status = LDi_clockGetTime(&ts, LD_CLOCK_REALTIME) == false)) {
            goto done;
        }

        ts.tv_sec += milliseconds / 1000;
        ts.tv_nsec += (milliseconds % 1000) * 1000 * 1000;
        if (ts.tv_nsec > 1000 * 1000 * 1000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000 * 1000 * 1000;
        }

        if ((status = pthread_cond_timedwait(cond, mutex, &ts)) != 0) {
            if (status != ETIMEDOUT) {
                LD_LOG_1(LD_LOG_CRITICAL, "pthread_cond_timedwait failed: %s",
                    strerror(status));

                goto done;
            } else {
                status = 0;
            }
        }
    #endif

  done:
    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_3(LD_LOG_TRACE, "LDi_cond_wait end %p %p %d",
            (void *)cond, (void *)mutex, milliseconds);
    #endif

    return status == 0;
}

static bool
LDi_cond_signal_imp(ld_cond_t *const cond)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_cond_signal start %p", (void *)cond);
    #endif

    LD_ASSERT(cond);

    #ifdef _WIN32
        WakeAllConditionVariable(cond);

        status = 0;
    #else
        if ((status = pthread_cond_broadcast(cond) != 0)) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_cond_broadcast failed: %s",
                strerror(status));
        }
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_cond_signal end %p", (void *)cond);
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    return status == 0;
}

static bool
LDi_cond_destroy_imp(ld_cond_t *const cond)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_cond_destroy start %p", (void *)cond);
    #endif

    LD_ASSERT(cond);

    #ifdef _WIN32
        /* windows has no destruction routine */
        status = 0;
    #else
        if ((status = pthread_cond_destroy(cond) != 0)) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_cond_destroy failed: %s",
                strerror(status));
        }
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_cond_destroy end %p", (void *)cond);
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    return status == 0;
}

static bool
LDi_cond_init_imp(ld_cond_t *const cond)
{
    int status;

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_cond_init start %p", (void *)cond);
    #endif

    LD_ASSERT(cond);

    #ifdef _WIN32
        InitializeConditionVariable(cond);

        status = 0;
    #else
        if ((status = pthread_cond_init(cond, NULL) != 0)) {
            LD_LOG_1(LD_LOG_CRITICAL, "pthread_cond_init failed: %s",
                strerror(status));
        }
    #endif

    #ifdef LAUNCHDARKLY_TRACE_CONCURRENCY
        LD_LOG_1(LD_LOG_TRACE, "LDi_cond_init end %p", (void *)cond);
    #endif

    #ifdef LAUNCHDARKLY_CONCURRENCY_ABORT
        LD_ASSERT(status == 0);
    #endif

    return status == 0;
}

ld_mutex_unary_t   LDi_mutex_init       = LDi_mutex_init_imp;
ld_mutex_unary_t   LDi_mutex_destroy    = LDi_mutex_destroy_imp;
ld_mutex_unary_t   LDi_mutex_lock       = LDi_mutex_lock_imp;
ld_mutex_unary_t   LDi_mutex_unlock     = LDi_mutex_unlock_imp;

ld_mutex_unary_t   LDi_mutex_nl_init    = LDi_mutex_init_nl_imp;
ld_mutex_unary_t   LDi_mutex_nl_destroy = LDi_mutex_destroy_nl_imp;
ld_mutex_unary_t   LDi_mutex_nl_lock    = LDi_mutex_lock_nl_imp;
ld_mutex_unary_t   LDi_mutex_nl_unlock  = LDi_mutex_unlock_nl_imp;

ld_thread_join_t   LDi_thread_join      = LDi_thread_join_imp;
ld_thread_create_t LDi_thread_create    = LDi_thread_create_imp;

ld_rwlock_unary_t  LDi_rwlock_init      = LDi_rwlock_init_imp;
ld_rwlock_unary_t  LDi_rwlock_destroy   = LDi_rwlock_destroy_imp;
ld_rwlock_unary_t  LDi_rwlock_rdlock    = LDi_rwlock_rdlock_imp;
ld_rwlock_unary_t  LDi_rwlock_wrlock    = LDi_rwlock_wrlock_imp;
ld_rwlock_unary_t  LDi_rwlock_rdunlock  = LDi_rwlock_rdunlock_imp;
ld_rwlock_unary_t  LDi_rwlock_wrunlock  = LDi_rwlock_wrunlock_imp;

ld_cond_unary_t    LDi_cond_init        = LDi_cond_init_imp;
ld_cond_wait_t     LDi_cond_wait        = LDi_cond_wait_imp;
ld_cond_unary_t    LDi_cond_signal      = LDi_cond_signal_imp;
ld_cond_unary_t    LDi_cond_destroy     = LDi_cond_destroy_imp;
