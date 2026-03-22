#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <arm_neon.h>

static long long gA = 0;
static long long gC = 0;
static long long gG = 0;
static long long gT = 0;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    const char *buf;
    long long start;
    long long end;
    long long A;
    long long C;
    long long G;
    long long T;
} thread_arg_t;

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void split_range(long long n, int threads, int tid, long long *start, long long *end)
{
    long long base = n / threads;
    long long rem = n % threads;
    long long extra = (tid < rem) ? 1 : 0;
    long long before = (tid < rem) ? tid : rem;

    *start = tid * base + before;
    *end = *start + base + extra;
}

char *buffer;
static long long load_entire_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return -1LL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1LL;
    }

    long long n = ftell(fp);
    if (n <= 0) {
        fclose(fp);
        return -1LL;
    }

    rewind(fp);

    buffer = (char *)malloc((size_t)n);
    if (!buffer) {
        fclose(fp);
        return -1LL;
    }

    if (fread(buffer, 1, (size_t)n, fp) != (size_t)n) {
        fclose(fp);
        free(buffer);
        buffer = NULL;
        return -1LL;
    }

    fclose(fp);
    return n;
}

static void count_scalar_range(const char *buf, long long start, long long end,
                               long long *A, long long *C, long long *G, long long *T)
{
    long long a = 0;
    long long c = 0;
    long long g = 0;
    long long t = 0;

    for (long long i = start; i < end; ++i) {
        char ch = buf[i];
        if (ch == 'A') {
            ++a;
        } else if (ch == 'C') {
            ++c;
        } else if (ch == 'G') {
            ++g;
        } else if (ch == 'T') {
            ++t;
        }
    }

    *A = a;
    *C = c;
    *G = g;
    *T = t;
}

static void count_simd_range(const char *buf, long long start, long long end,
                             long long *A, long long *C, long long *G, long long *T)
{
    long long a = 0;
    long long c = 0;
    long long g = 0;
    long long t = 0;
    long long i = start;

    uint8x16_t va = vdupq_n_u8('A');
    uint8x16_t vc = vdupq_n_u8('C');
    uint8x16_t vg = vdupq_n_u8('G');
    uint8x16_t vt = vdupq_n_u8('T');

    for (; i + 15 < end; i += 16) {
        uint8x16_t x = vld1q_u8((const uint8_t *)(buf + i));
        a += vaddvq_u8(vshrq_n_u8(vceqq_u8(x, va), 7));
        c += vaddvq_u8(vshrq_n_u8(vceqq_u8(x, vc), 7));
        g += vaddvq_u8(vshrq_n_u8(vceqq_u8(x, vg), 7));
        t += vaddvq_u8(vshrq_n_u8(vceqq_u8(x, vt), 7));

    }

    for (; i < end; ++i) {
        char ch = buf[i];
        if (ch == 'A') {
            ++a;
        } else if (ch == 'C') {
            ++c;
        } else if (ch == 'G') {
            ++g;
        } else if (ch == 'T') {
            ++t;
        }
    }

    *A = a;
    *C = c;
    *G = g;
    *T = t;
}

static void *worker_mt_global(void *p)
{
    thread_arg_t *arg = (thread_arg_t *)p;
    long long a, c, g, t;

    count_scalar_range(arg->buf, arg->start, arg->end, &a, &c, &g, &t);

    pthread_mutex_lock(&g_lock);
    gA += a;
    gC += c;
    gG += g;
    gT += t;
    pthread_mutex_unlock(&g_lock);
    return NULL;
}

static void *worker_simd_mt_local(void *p)
{
    thread_arg_t *arg = (thread_arg_t *)p;
    count_simd_range(arg->buf, arg->start, arg->end, &arg->A, &arg->C, &arg->G, &arg->T);
    return NULL;
}

static void run_multithreading(const char *buf, long long n, int threads,
                               long long *A, long long *C, long long *G, long long *T)
{
    pthread_t ids[threads];
    thread_arg_t args[threads];

    gA = gC = gG = gT = 0;

    for (int i = 0; i < threads; ++i) {
        split_range(n, threads, i, &args[i].start, &args[i].end);
        args[i].buf = buf;
        pthread_create(&ids[i], NULL, worker_mt_global, &args[i]);
    }

    for (int i = 0; i < threads; ++i) {
        pthread_join(ids[i], NULL);
    }

    *A = gA;
    *C = gC;
    *G = gG;
    *T = gT;
}

static void run_simd_multithreading(const char *buf, long long n, int threads,
                                    long long *A, long long *C, long long *G, long long *T)
{
    pthread_t ids[threads];
    thread_arg_t args[threads];
    long long a = 0;
    long long c = 0;
    long long g = 0;
    long long t = 0;

    for (int i = 0; i < threads; ++i) {
        split_range(n, threads, i, &args[i].start, &args[i].end);
        args[i].buf = buf;
        args[i].A = 0;
        args[i].C = 0;
        args[i].G = 0;
        args[i].T = 0;
        pthread_create(&ids[i], NULL, worker_simd_mt_local, &args[i]);
    }

    for (int i = 0; i < threads; ++i) {
        pthread_join(ids[i], NULL);
        printf("%lld", args[i].A);
        a += args[i].A;
        c += args[i].C;
        g += args[i].G;
        t += args[i].T;
    }

    *A = a;
    *C = c;
    *G = g;
    *T = t;
}

int main(int argc, char **argv)
{
    const char *dna_file = "HW4/dna_data.bin";
    int threads = 4;
    long long n = 0;

    n = load_entire_file(dna_file);
    if (n <= 0 || !buffer) {
        printf("Failed to read file: %s\n", dna_file);
        return 1;
    }

    long long sA, sC, sG, sT;
    long long mA, mC, mG, mT;
    long long vA, vC, vG, vT;
    long long vmA, vmC, vmG, vmT;

    double t1 = get_time();
    count_scalar_range(buffer, 0, n, &sA, &sC, &sG, &sT);
    double t2 = get_time();

    double t3 = get_time();
    run_multithreading(buffer, n, threads, &mA, &mC, &mG, &mT);
    double t4 = get_time();

    double t5 = get_time();
    count_simd_range(buffer, 0, n, &vA, &vC, &vG, &vT);
    double t6 = get_time();

    double t7 = get_time();
    run_simd_multithreading(buffer, n, threads, &vmA, &vmC, &vmG, &vmT);
    double t8 = get_time();

    printf("DNA file: %s\n", dna_file);
    printf("DNA size: %.2f MB\n", (double)n / (1024.0 * 1024.0));
    printf("Threads used: %d\n\n", threads);

    printf("Counts (A C G T):\n");
    printf("Scalar:                %lld %lld %lld %lld\n", sA, sC, sG, sT);
    printf("Multithreading:        %lld %lld %lld %lld\n", mA, mC, mG, mT);
    printf("SIMD:                  %lld %lld %lld %lld\n", vA, vC, vG, vT);
    printf("SIMD + Multithreading: %lld %lld %lld %lld\n\n", vmA, vmC, vmG, vmT);

    printf("Scalar time:                %.6f sec\n", t2 - t1);
    printf("Multithreading time:        %.6f sec\n", t4 - t3);
    printf("SIMD time:                  %.6f sec\n", t6 - t5);
    printf("SIMD + Multithreading time: %.6f sec\n", t8 - t7);

    free(buffer);
    buffer = NULL;
    return 0;
}
