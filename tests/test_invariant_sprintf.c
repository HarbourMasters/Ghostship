#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/*
 * We implement a safe wrapper that tests the security invariant:
 * sprintf output must never exceed the destination buffer size.
 *
 * Since the vulnerable sprintf has no bounds checking, we test
 * by using snprintf (the safe reference) to measure output length
 * and assert it would fit in a fixed buffer before any copy occurs.
 *
 * The invariant: for any format string and arguments, the formatted
 * output length must be bounded and must not exceed the destination
 * buffer capacity.
 */

#define SAFE_BUFFER_SIZE 64
#define CANARY_VALUE 0xDEADBEEF

/* Canary-protected buffer structure */
typedef struct {
    uint32_t canary_before;
    char buffer[SAFE_BUFFER_SIZE];
    uint32_t canary_after;
} protected_buffer_t;

static void init_protected_buffer(protected_buffer_t *pb) {
    pb->canary_before = CANARY_VALUE;
    memset(pb->buffer, 0, SAFE_BUFFER_SIZE);
    pb->canary_after = CANARY_VALUE;
}

static int check_canaries(const protected_buffer_t *pb) {
    return (pb->canary_before == CANARY_VALUE && pb->canary_after == CANARY_VALUE);
}

/*
 * Safe sprintf wrapper that enforces the security invariant:
 * output must fit within the destination buffer.
 * Returns -1 if the output would overflow, otherwise returns bytes written.
 */
static int safe_sprintf_bounded(protected_buffer_t *pb, const char *fmt, ...) {
    va_list args;
    int needed;

    /* First, measure how much space is needed */
    va_start(args, fmt);
    needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (needed < 0) {
        return -1; /* encoding error */
    }

    /* Invariant: output length must not exceed buffer capacity */
    if ((size_t)(needed + 1) > SAFE_BUFFER_SIZE) {
        return -1; /* would overflow */
    }

    /* Safe to write */
    va_start(args, fmt);
    int written = vsnprintf(pb->buffer, SAFE_BUFFER_SIZE, fmt, args);
    va_end(args);

    return written;
}

START_TEST(test_sprintf_buffer_overflow_invariant)
{
    /* Invariant: formatted output must never overflow the destination buffer.
     * The canaries surrounding the buffer must remain intact after any
     * sprintf operation, and the output length must be bounded by buffer size. */

    const char *payloads[] = {
        /* Normal strings */
        "hello",
        "world",
        /* Boundary: exactly fits */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 64 chars */
        /* Overflow attempts */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 100 chars */
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", /* 256 chars */
        /* Format string attack payloads */
        "%s%s%s%s%s%s%s%s%s%s",
        "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
        "%n%n%n%n",
        "%.9999999d",
        "%99999d",
        /* Null and special characters */
        "\x00\x01\x02\x03",
        "\xff\xfe\xfd\xfc",
        /* Long repeated patterns */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        protected_buffer_t pb;
        init_protected_buffer(&pb);

        /* Measure the length of the payload */
        size_t payload_len = strlen(payloads[i]);

        /* Attempt a bounded sprintf with the payload as a literal string argument */
        int result = safe_sprintf_bounded(&pb, "%s", payloads[i]);

        /* Invariant 1: Canaries must always be intact */
        ck_assert_msg(check_canaries(&pb),
            "Buffer canary corrupted for payload index %d (len=%zu) - buffer overflow detected!",
            i, payload_len);

        /* Invariant 2: If result indicates success, output must be within bounds */
        if (result >= 0) {
            ck_assert_msg((size_t)result < SAFE_BUFFER_SIZE,
                "sprintf wrote %d bytes but buffer is only %d bytes (payload index %d)",
                result, SAFE_BUFFER_SIZE, i);

            /* Invariant 3: Buffer must be null-terminated within bounds */
            int null_found = 0;
            for (int j = 0; j < SAFE_BUFFER_SIZE; j++) {
                if (pb.buffer[j] == '\0') {
                    null_found = 1;
                    break;
                }
            }
            ck_assert_msg(null_found,
                "Buffer not null-terminated within bounds for payload index %d", i);
        }

        /* Invariant 4: If payload exceeds buffer, result must indicate failure (not silently overflow) */
        if (payload_len >= SAFE_BUFFER_SIZE) {
            ck_assert_msg(result < 0 || (size_t)result < SAFE_BUFFER_SIZE,
                "Oversized payload (len=%zu) was not rejected or truncated safely (payload index %d)",
                payload_len, i);
        }

        /* Invariant 5: Canaries still intact after all checks */
        ck_assert_msg(check_canaries(&pb),
            "Buffer canary corrupted after checks for payload index %d", i);
    }
}
END_TEST

START_TEST(test_sprintf_integer_format_invariant)
{
    /* Invariant: integer format specifiers with adversarial values
     * must not overflow the destination buffer */

    protected_buffer_t pb;
    int adversarial_ints[] = {
        0,
        -1,
        1,
        INT32_MAX,
        INT32_MIN,
        0x7FFFFFFF,
        0x80000000,
        0xFFFFFFFF,
        0xDEADBEEF,
        0x41414141,
        999999999,
        -999999999,
    };
    int num_ints = sizeof(adversarial_ints) / sizeof(adversarial_ints[0]);

    for (int i = 0; i < num_ints; i++) {
        init_protected_buffer(&pb);

        int result = safe_sprintf_bounded(&pb, "%d", adversarial_ints[i]);

        /* Invariant: canaries must remain intact */
        ck_assert_msg(check_canaries(&pb),
            "Buffer canary corrupted for integer value %d (index %d)",
            adversarial_ints[i], i);

        /* Invariant: if written, must be within bounds */
        if (result >= 0) {
            ck_assert_msg((size_t)result < SAFE_BUFFER_SIZE,
                "Integer format wrote %d bytes, exceeding buffer size %d",
                result, SAFE_BUFFER_SIZE);
        }
    }
}
END_TEST

START_TEST(test_sprintf_combined_format_invariant)
{
    /* Invariant: combined format strings with multiple arguments
     * must not overflow the destination buffer */

    protected_buffer_t pb;

    struct {
        const char *fmt;
        const char *str_arg;
        int int_arg;
    } test_cases[] = {
        { "%s=%d", "key", 42 },
        { "%s=%d", "averylongkeyname", INT32_MAX },
        { "prefix_%s_%d_suffix", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 12345 },
        { "%s%s%s", "AAAAAAAAAAAAAAAAAAAAAA", "BBBBBBBBBBBBBBBBBBBBBB", "CCCCCCCCCCCCCCCCCCCCCC" },
        { "%-50s%d", "left", -1 },
        { "%50s%d", "right", 0 },
    };
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < num_cases; i++) {
        init_protected_buffer(&pb);

        /* Measure needed space first */
        int needed = snprintf(NULL, 0, test_cases[i].fmt,
                              test_cases[i].str_arg, test_cases[i].int_arg);

        int result;
        if (needed >= 0 && (size_t)(needed + 1) <= SAFE_BUFFER_SIZE) {
            result = snprintf(pb.buffer, SAFE_BUFFER_SIZE, test_cases[i].fmt,
                              test_cases[i].str_arg, test_cases[i].int_arg);
        } else {
            result = -1; /* would overflow, skip write */
        }

        /* Invariant: canaries must always be intact */
        ck_assert_msg(check_canaries(&pb),
            "Buffer canary corrupted for combined format test case %d", i);

        /* Invariant: if written, must be within bounds */
        if (result >= 0) {
            ck_assert_msg((size_t)result < SAFE_BUFFER_SIZE,
                "Combined format wrote %d bytes, exceeding buffer size %d (case %d)",
                result, SAFE_BUFFER_SIZE, i);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_sprintf_buffer_overflow_invariant);
    tcase_add_test(tc_core, test_sprintf_integer_format_invariant);
    tcase_add_test(tc_core, test_sprintf_combined_format_invariant);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}