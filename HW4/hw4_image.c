#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <arm_neon.h>
#include <string.h>

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static long long g_width = 0;
static long long g_height = 0;
static long long g_maxval = 255;

typedef struct {
    char *buf;
    long long start;
    long long end;
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

//next 4 AI
char *buffer;
static int read_ppm_token(FILE *fp, char *out, size_t out_cap)
{
    int c = fgetc(fp);

    while (c != EOF) {
        if (isspace((unsigned char)c)) {
            c = fgetc(fp);
            continue;
        }
        if (c == '#') {
            do {
                c = fgetc(fp);
            } while (c != EOF && c != '\n' && c != '\r');
            c = fgetc(fp);
            continue;
        }
        break;
    }

    if (c == EOF) {
        return 0;
    }

    size_t i = 0;
    while (c != EOF && !isspace((unsigned char)c) && c != '#') {
        if (i + 1 < out_cap) {
            out[i++] = (char)c;
        }
        c = fgetc(fp);
    }
    out[i] = '\0';

    if (c == '#') {
        do {
            c = fgetc(fp);
        } while (c != EOF && c != '\n' && c != '\r');
    }

    return i > 0;
}

static long long load_entire_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return -1LL;
    }

    char tok[32];
    if (!read_ppm_token(fp, tok, sizeof(tok)) || tok[0] != 'P' || tok[1] != '6' || tok[2] != '\0') {
        fclose(fp);
        return -1LL;
    }

    if (!read_ppm_token(fp, tok, sizeof(tok))) {
        fclose(fp);
        return -1LL;
    }
    long long width = atoll(tok);
    if (width <= 0) {
        fclose(fp);
        return -1LL;
    }

    if (!read_ppm_token(fp, tok, sizeof(tok))) {
        fclose(fp);
        return -1LL;
    }
    long long height = atoll(tok);
    if (height <= 0) {
        fclose(fp);
        return -1LL;
    }

    if (!read_ppm_token(fp, tok, sizeof(tok))) {
        fclose(fp);
        return -1LL;
    }
    long long maxval = atoll(tok);
    if (maxval <= 0 || maxval > 255) {
        fclose(fp);
        return -1LL;
    }

    unsigned long long pixels = (unsigned long long)width * (unsigned long long)height;
    if (pixels == 0ULL || pixels > ((unsigned long long)SIZE_MAX / 3ULL)) {
        fclose(fp);
        return -1LL;
    }

    size_t n = (size_t)(pixels * 3ULL);
    buffer = (char *)malloc(n);
    if (!buffer) {
        fclose(fp);
        return -1LL;
    }

    if (fread(buffer, 1, n, fp) != n) {
        fclose(fp);
        free(buffer);
        buffer = NULL;
        return -1LL;
    }

    fclose(fp);
    g_width = width;
    g_height = height;
    g_maxval = maxval;
    return (long long)n;
}

static int save_ppm(const char *path, const char *buf, long long n)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        return -1;
    }

    if (fprintf(fp, "P6\n%lld %lld\n%lld\n", g_width, g_height, g_maxval) < 0) {
        fclose(fp);
        return -1;
    }

    if (fwrite(buf, 1, (size_t)n, fp) != (size_t)n) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

static void make_out_name(char *dst, size_t cap, const char *prefix)
{
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char d[16];
    strftime(d, sizeof(d), "%Y%m%d", &tm_now);
    snprintf(dst, cap, "HW4/%s_%s.ppm", prefix, d);
}

static void scalar(char *buf, long long start, long long end)
{
    if (start < 0) {
        start = 0;
    }
    if (end < start) {
        return;
    }

    start -= start % 3;
    end -= end % 3;

    for (long long i = start; i < end; i += 3) {
        float r = (float)(uint8_t)buf[i];
        float g = (float)(uint8_t)buf[i + 1];
        float b = (float)(uint8_t)buf[i + 2];
        uint8_t y = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
        buf[i] = (char)y;
        buf[i + 1] = (char)y;
        buf[i + 2] = (char)y;
    }
}

static void simd(char *buf, long long start, long long end)
{
    if (start < 0) {
        start = 0;
    }
    if (end < start) {
        return;
    }

    start -= start % 3;
    end -= end % 3;

    uint8_t *p = (uint8_t *)buf + start;
    long long bytes = end - start;
    long long i = 0;

    for (; i + 24 <= bytes; i += 24) {
        uint8x8x3_t rgb = vld3_u8(p + i);
        uint16x8_t r16 = vmovl_u8(rgb.val[0]);
        uint16x8_t g16 = vmovl_u8(rgb.val[1]);
        uint16x8_t b16 = vmovl_u8(rgb.val[2]);

        uint32x4_t r_lo = vmovl_u16(vget_low_u16(r16));
        uint32x4_t g_lo = vmovl_u16(vget_low_u16(g16));
        uint32x4_t b_lo = vmovl_u16(vget_low_u16(b16));
        uint32x4_t r_hi = vmovl_u16(vget_high_u16(r16));
        uint32x4_t g_hi = vmovl_u16(vget_high_u16(g16));
        uint32x4_t b_hi = vmovl_u16(vget_high_u16(b16));

        float32x4_t rf_lo = vcvtq_f32_u32(r_lo);
        float32x4_t gf_lo = vcvtq_f32_u32(g_lo);
        float32x4_t bf_lo = vcvtq_f32_u32(b_lo);
        float32x4_t rf_hi = vcvtq_f32_u32(r_hi);
        float32x4_t gf_hi = vcvtq_f32_u32(g_hi);
        float32x4_t bf_hi = vcvtq_f32_u32(b_hi);

        float32x4_t yf_lo = vmulq_n_f32(rf_lo, 0.299f);
        yf_lo = vmlaq_n_f32(yf_lo, gf_lo, 0.587f);
        yf_lo = vmlaq_n_f32(yf_lo, bf_lo, 0.114f);

        float32x4_t yf_hi = vmulq_n_f32(rf_hi, 0.299f);
        yf_hi = vmlaq_n_f32(yf_hi, gf_hi, 0.587f);
        yf_hi = vmlaq_n_f32(yf_hi, bf_hi, 0.114f);

        uint32x4_t y32_lo = vcvtq_u32_f32(yf_lo);
        uint32x4_t y32_hi = vcvtq_u32_f32(yf_hi);
        uint16x8_t y16 = vcombine_u16(vqmovn_u32(y32_lo), vqmovn_u32(y32_hi));
        uint8x8_t y = vqmovn_u16(y16);

        uint8x8x3_t out;
        out.val[0] = y;
        out.val[1] = y;
        out.val[2] = y;
        vst3_u8(p + i, out);
    }

    for (; i < bytes; i += 3) {
        float r = (float)p[i];
        float g = (float)p[i + 1];
        float b = (float)p[i + 2];
        uint8_t y = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
        p[i] = y;
        p[i + 1] = y;
        p[i + 2] = y;
    }
}

static void *threaded_scalar(void *p)
{
    thread_arg_t *arg = (thread_arg_t *)p;
    scalar(arg->buf, arg->start, arg->end);
    return NULL;
}

static void *threaded_simd(void *p)
{
    thread_arg_t *arg = (thread_arg_t *)p;
    simd(arg->buf, arg->start, arg->end);
    return NULL;
}

static void run_multithreading(char *buf, long long n, int threads)
{
    if (threads <= 1) {
        scalar(buf, 0, n);
        return;
    }

    pthread_t ids[threads];
    thread_arg_t args[threads];

    int created = 0;
    int failed = 0;

    for (int i = 0; i < threads; ++i) {
        split_range(n, threads, i, &args[i].start, &args[i].end);
        args[i].buf = buf;

        if (pthread_create(&ids[i], NULL, threaded_scalar, &args[i]) != 0) {
            failed = 1;
            break;
        }
        created++;
    }

    for (int i = 0; i < created; ++i) {
        pthread_join(ids[i], NULL);
    }

    if (failed) {
        scalar(buf, 0, n);
        return;
    }

    for (int i = 0; i < threads; ++i) {
        pthread_join(ids[i], NULL);
    }
}

static void run_simd_multithreading(char *buf, long long n, int threads)
{
    if (threads <= 1) {
        simd(buf, 0, n);
        return;
    }

    pthread_t ids[threads];
    thread_arg_t args[threads];
    int created = 0;
    int failed = 0;

    for (int i = 0; i < threads; ++i) {
        split_range(n, threads, i, &args[i].start, &args[i].end);
        args[i].buf = buf;
        if (pthread_create(&ids[i], NULL, threaded_simd, &args[i]) != 0) {
            failed = 1;
            break;
        }
        created++;
    }

    for (int i = 0; i < created; ++i) {
        pthread_join(ids[i], NULL);
    }

    if (failed) {
        simd(buf, 0, n);
    }
}

int main(int argc, char **argv)
{
    const char *ppm_file = "HW4/image.ppm";
    int threads = 4;
    long long n = 0;
    
    n = load_entire_file(ppm_file);
    if (n <= 0 || !buffer) {
        printf("Failed to read file: %s\n", ppm_file);
        return 1;
    }

    char *buf_scalar = (char *)malloc((size_t)n);
    char *buf_mt = (char *)malloc((size_t)n);
    char *buf_simd = (char *)malloc((size_t)n);
    char *buf_simd_mt = (char *)malloc((size_t)n);
    if (!buf_scalar || !buf_mt || !buf_simd || !buf_simd_mt) {
        printf("Failed to allocate work buffers\n");
        free(buf_scalar);
        free(buf_mt);
        free(buf_simd);
        free(buf_simd_mt);
        free(buffer);
        buffer = NULL;
        return 1;
    }
    memcpy(buf_scalar, buffer, (size_t)n);
    memcpy(buf_mt, buffer, (size_t)n);
    memcpy(buf_simd, buffer, (size_t)n);
    memcpy(buf_simd_mt, buffer, (size_t)n);

    double t1 = get_time();
    scalar(buf_scalar, 0, n);
    double t2 = get_time();
    char out_scalar[128];
    make_out_name(out_scalar, sizeof(out_scalar), "out_scalar");
    save_ppm(out_scalar, buf_scalar, n);

    double t3 = get_time();
    run_multithreading(buf_mt, n, threads);
    double t4 = get_time();
    char out_mt[128];
    make_out_name(out_mt, sizeof(out_mt), "out_threadedscalar");
    save_ppm(out_mt, buf_mt, n);

    double t5 = get_time();
    simd(buf_simd, 0, n);
    double t6 = get_time();
    char out_simd[128];
    make_out_name(out_simd, sizeof(out_simd), "out_simd");
    save_ppm(out_simd, buf_simd, n);

    double t7 = get_time();
    run_simd_multithreading(buf_simd_mt, n, threads);
    double t8 = get_time();
    char out_simd_mt[128];
    make_out_name(out_simd_mt, sizeof(out_simd_mt), "out_threadedsimd");
    save_ppm(out_simd_mt, buf_simd_mt, n);


    printf("PPM file: %s\n", ppm_file);
    printf("Threads used: %d\n\n", threads);

    printf("Scalar time:                %.6f sec\n", t2 - t1);
    printf("Multithreading time:        %.6f sec\n", t4 - t3);
    printf("SIMD time:                  %.6f sec\n", t6 - t5);
    printf("SIMD + Multithreading time: %.6f sec\n", t8 - t7);

    free(buf_scalar);
    free(buf_mt);
    free(buf_simd);
    free(buf_simd_mt);
    free(buffer);
    buffer = NULL;
    return 0;
}
