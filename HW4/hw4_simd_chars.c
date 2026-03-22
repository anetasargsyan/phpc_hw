#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arm_neon.h>

#define DEFAULT_BUFFER_MB 256u
#define MIN_RECOMMENDED_MB 200u

struct worker_arg {
    char *buf;
    size_t start;
    size_t end;
};

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void split_range(size_t n, int threads, int tid, size_t *start, size_t *end)
{
    size_t base = n / (size_t)threads;
    size_t rem = n % (size_t)threads;
    size_t extra = ((size_t)tid < rem) ? 1u : 0u;
    size_t before = ((size_t)tid < rem) ? (size_t)tid : rem;

    *start = (size_t)tid * base + before;
    *end = *start + base + extra;
}

static void fill_mixed_buffer(char *buf, size_t n, uint32_t seed)
{
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,;:!?-_()/\\[]{}@#$^&*+=\t\n";

    srand((unsigned int)seed);
    size_t charset_len = sizeof(charset) - 1;

    for (size_t i = 0; i < n; ++i) {
        buf[i] = charset[(size_t)rand() % charset_len];
    }
}

static void uppercase_scalar_range(char *buf, size_t start, size_t end)
{
    for (size_t i = start; i < end; ++i) {
        unsigned char c = (unsigned char)buf[i];
        if (c >= 'a' && c <= 'z') {
            buf[i] = (char)(c - ('a' - 'A'));
        }
    }
}

static void uppercase_simd_range(char *buf, size_t start, size_t end)
{
    size_t n = end - start;
    uint8_t *p = (uint8_t *)buf + start;
    size_t i = 0;

    const uint8x16_t va = vdupq_n_u8('a');
    const uint8x16_t vz = vdupq_n_u8('z');
    const uint8x16_t bit = vdupq_n_u8(0x20);

    for (; i + 15 < n; i += 16) {
        uint8x16_t v = vld1q_u8(p + i);
        uint8x16_t ge_a = vcgeq_u8(v, va);
        uint8x16_t le_z = vcleq_u8(v, vz);
        uint8x16_t mask = vandq_u8(ge_a, le_z);
        uint8x16_t delta = vandq_u8(mask, bit);
        uint8x16_t out = vsubq_u8(v, delta);
        vst1q_u8(p + i, out);
    }
    
    for (; i < n; ++i) {
        if (p[i] >= 'a' && p[i] <= 'z') {
            p[i] = (uint8_t)(p[i] - ('a' - 'A'));
        }
    }
}

static void *thread_scalar_worker(void *arg)
{
    struct worker_arg *a = (struct worker_arg *)arg;
    uppercase_scalar_range(a->buf, a->start, a->end);
    return NULL;
}

static void *thread_simd_worker(void *arg)
{
    struct worker_arg *a = (struct worker_arg *)arg;
    uppercase_simd_range(a->buf, a->start, a->end);
    return NULL;
}

static void run_simd(char *buf, size_t n)
{
    uppercase_simd_range(buf, 0, n);
}

int main()
{
    size_t buffer_mb = DEFAULT_BUFFER_MB;
    int threads = 8;
    buffer_mb = 250;
    threads = 4;

    size_t n = buffer_mb * 1024u * 1024u;

    char *src = (char *)malloc(n);
    char *buf_mt = (char *)malloc(n);
    char *buf_simd = (char *)malloc(n);
    char *buf_simd_mt = (char *)malloc(n);

    if (!src || !buf_mt || !buf_simd || !buf_simd_mt) {
        fprintf(stderr, "Failed to allocate buffers (size: %zu bytes each).\n", n);
        free(src);
        free(buf_mt);
        free(buf_simd);
        free(buf_simd_mt);
        return 1;
    }

    fill_mixed_buffer(src, n, 0xC0FFEEu);
    memcpy(buf_mt, src, n);
    memcpy(buf_simd, src, n);
    memcpy(buf_simd_mt, src, n);

    double t0 = get_time();

    pthread_t *th = (pthread_t *)malloc((size_t)threads * sizeof(*th));
    struct worker_arg *args = (struct worker_arg *)malloc((size_t)threads * sizeof(*args));

    if (!th || !args) {
        fprintf(stderr, "Allocation failed in run_multithreading\n");
        free(th);
        free(args);
        return 1;
    }

    for (int t = 0; t < threads; ++t) {
        split_range(n, threads, t, &args[t].start, &args[t].end);
        args[t].buf = buf_mt;

        if (pthread_create(&th[t], NULL, thread_scalar_worker, &args[t]) != 0) {
            perror("pthread_create");
            free(th);
            free(args);
            return 1;
        }
    }

    for (int t = 0; t < threads; ++t) {
        if (pthread_join(th[t], NULL) != 0) {
            perror("pthread_join");
            free(th);
            free(args);
            return 1;
        }
    }

    free(th);
    free(args);
    double t1 = get_time();

    double t2 = get_time();
    run_simd(buf_simd, n);
    double t3 = get_time();

    double t4 = get_time();

    pthread_t *th_1 = (pthread_t *)malloc((size_t)threads * sizeof(*th_1));
    struct worker_arg *args_1 = (struct worker_arg *)malloc((size_t)threads * sizeof(*args_1));

    if (!th_1 || !args_1) {
        fprintf(stderr, "Allocation failed in run_simd_multithreading\n");
        free(th_1);
        free(args_1);
        return 1;
    }

    for (int t = 0; t < threads; ++t) {
        split_range(n, threads, t, &args_1[t].start, &args_1[t].end);
        args_1[t].buf = buf_simd_mt;

        if (pthread_create(&th_1[t], NULL, thread_simd_worker, &args_1[t]) != 0) {
            perror("pthread_create");
            free(th_1);
            free(args_1);
            return 1;
        }
    }

    for (int t = 0; t < threads; ++t) {
        if (pthread_join(th[t], NULL) != 0) {
            perror("pthread_join");
            free(th_1);
            free(args_1);
            return 1;
        }
    }

    free(th_1);
    free(args_1);

    double t5 = get_time();
    
    printf("Buffer size: %zu MB\n", buffer_mb);
    printf("Threads used: %d\n\n", threads);
    printf("Multithreading time:      %.6f sec\n", t1 - t0);
    printf("SIMD time:                %.6f sec\n", t3 - t2);
    printf("SIMD + Multithreading:    %.6f sec\n", t5 - t4);
    free(buf_mt);
    free(buf_simd);
    free(buf_simd_mt);

    double t6 = get_time();

    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)src[i];
        if (c >= 'a' && c <= 'z') {
            src[i] = (char)(c - ('a' - 'A'));
        }
    }

    double t7 = get_time();
  
    printf("Personal :) scalar:    %.6f sec\n", t7 - t6);
    free(src);
    return 0;
}
