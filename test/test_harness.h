#pragma once
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// ---------------------------------------------------------------------------
// stderr capture — redirects stderr to a pipe, reads it back into a buffer
// ---------------------------------------------------------------------------

static int    _stderr_pipe[2];
static int    _stderr_saved;
static char   _stderr_buf[4096];
static size_t _stderr_len;

#define CAPTURE_STDERR_BEGIN()                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        fflush(stderr);                                                                                                \
        _stderr_saved = dup(STDERR_FILENO);                                                                            \
        pipe(_stderr_pipe);                                                                                            \
        dup2(_stderr_pipe[1], STDERR_FILENO);                                                                          \
        close(_stderr_pipe[1]);                                                                                        \
    } while (0)

#define CAPTURE_STDERR_END()                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        fflush(stderr);                                                                                                \
        dup2(_stderr_saved, STDERR_FILENO);                                                                            \
        close(_stderr_saved);                                                                                          \
        _stderr_len             = read(_stderr_pipe[0], _stderr_buf, sizeof(_stderr_buf) - 1);                         \
        if (_stderr_len == (size_t) -1) _stderr_len = 0;                                                               \
        _stderr_buf[_stderr_len] = '\0';                                                                               \
        close(_stderr_pipe[0]);                                                                                        \
    } while (0)

#define ASSERT_STDERR_CONTAINS(substring)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (strstr(_stderr_buf, substring) == NULL)                                                                    \
        {                                                                                                              \
            printf("FAIL\n    %s:%d: stderr missing \"%s\"\n    got: \"%s\"\n", __FILE__, __LINE__, substring,         \
                   _stderr_buf);                                                                                       \
            tests_failed++;                                                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define ASSERT_STDERR_EMPTY()                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if (_stderr_len > 0)                                                                                           \
        {                                                                                                              \
            printf("FAIL\n    %s:%d: expected no stderr, got: \"%s\"\n", __FILE__, __LINE__, _stderr_buf);             \
            tests_failed++;                                                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define RUN_TEST(fn)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        tests_run++;                                                                                                   \
        printf("  %-55s", #fn);                                                                                        \
        fn();                                                                                                          \
        if (tests_run == tests_passed + tests_failed)                                                                  \
        {                                                                                                              \
            /* test already recorded a failure */                                                                       \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            tests_passed++;                                                                                            \
            printf("PASS\n");                                                                                          \
        }                                                                                                              \
    } while (0)

#define ASSERT_EQ(actual, expected)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        long long _a = (long long)(actual), _e = (long long)(expected);                                                \
        if (_a != _e)                                                                                                  \
        {                                                                                                              \
            printf("FAIL\n    %s:%d: expected %lld, got %lld\n", __FILE__, __LINE__, _e, _a);                          \
            tests_failed++;                                                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define ASSERT_NOT_NULL(ptr)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((ptr) == NULL)                                                                                             \
        {                                                                                                              \
            printf("FAIL\n    %s:%d: expected non-NULL\n", __FILE__, __LINE__);                                        \
            tests_failed++;                                                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define ASSERT_NULL(ptr)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((ptr) != NULL)                                                                                             \
        {                                                                                                              \
            printf("FAIL\n    %s:%d: expected NULL\n", __FILE__, __LINE__);                                            \
            tests_failed++;                                                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define ASSERT_FLOAT_EQ(actual, expected)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        float _a = (actual), _e = (expected);                                                                          \
        if (fabsf(_a - _e) > 1e-6f)                                                                                   \
        {                                                                                                              \
            printf("FAIL\n    %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, _e, _a);                          \
            tests_failed++;                                                                                            \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define TEST_SUMMARY()                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        printf("\n%d/%d tests passed", tests_passed, tests_run);                                                       \
        if (tests_failed > 0)                                                                                          \
            printf(", %d FAILED", tests_failed);                                                                       \
        printf("\n");                                                                                                  \
        return tests_failed > 0 ? 1 : 0;                                                                              \
    } while (0)
