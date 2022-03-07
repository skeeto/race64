#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define VERSION "1.0.0"

#define GROUPS_PER_LINE   19         // 1 group = 3 octets, 4 base64 digits
#define LINES_PER_BLOCK   (1 << 14)  // tweaked experimentally

#if __clang__
#  define UNROLL _Pragma("unroll")
#else
#  define UNROLL
#endif

/**
 * Encode from one file stream to another as fast as possible. Input and
 * output errors are detected by not communicated by this interface, so
 * check the streams' error indicators (e.g. ferror()) afterward.
 */
static void
encode(FILE *fin, FILE *fout)
{
    static unsigned char base64[] = {
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
        0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
        0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f,
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
        0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
        0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f,
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
        0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
        0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f,
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
        0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
        0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f
    };
    static unsigned char in[LINES_PER_BLOCK][GROUPS_PER_LINE * 3];
    static unsigned char out[LINES_PER_BLOCK][GROUPS_PER_LINE * 4 + 1];

    /* Pre-write all the newlines */
    for (int i = 0; i < LINES_PER_BLOCK; i++)
        out[i][sizeof(out[0]) - 1] = 0x0a;

    for (;;) {
        size_t len = fread(in, 1, sizeof(in), fin);
        if (!len) break;
        memset((char *)in + len, 0, sizeof(in) - len);

        /* Convert entire block at once with minimal branching */
        int n;
        #pragma omp parallel for
        for (n = 0; n < LINES_PER_BLOCK; n++) {
            unsigned char *pi = in[n];
            unsigned char *po = out[n];
            /* Unrolling this inner loop has HUGE performance gains. */
            UNROLL
            for (int i = 0; i < GROUPS_PER_LINE; i++) {
                po[i*4+0] = base64[(pi[i*3+0] >> 2)];
                po[i*4+1] = base64[(pi[i*3+0] << 4 | pi[i*3+1] >> 4) & 0xff];
                po[i*4+2] = base64[(pi[i*3+1] << 2 | pi[i*3+2] >> 6) & 0xff];
                po[i*4+3] = base64[(pi[i*3+1] << 6 | pi[i*3+2] >> 0) & 0xff];
            }
        }

        if (len < sizeof(in)) {
            /* Do a partial final write */
            int nout = (len + sizeof(in[0]) - 1) / sizeof(in[0]);
            int linelen = len % sizeof(in[0]);
            int end = (linelen + 2) / 3 * 4;
            if (end) {
                /* Truncate last line */
                switch (linelen % 3) {
                    case 1: out[nout - 1][end - 2] = 0x3d; /* fallthrough */
                    case 2: out[nout - 1][end - 1] = 0x3d; /* fallthrough */
                    case 0: out[nout - 1][end - 0] = 0x0a;
                }
                fwrite(out, (nout - 1) * sizeof(out[0]) + end + 1, 1, fout);
            } else {
                /* Last line is whole */
                fwrite(out, nout * sizeof(out[0]), 1, fout);
            }
            break;
        } else if (!fwrite(out, sizeof(out), 1, fout)) {
            /* Output error */
            break;
        }
    }
}

/**
 * Decode from one file stream to another as fast as possible. Returns the
 * line number of the earliest detected syntax error, or 0 for success.
 * However, if input and output errors are not communicated by this
 * interface, so check the streams' error indicator (e.g. ferror())
 * afterward.
 */
static unsigned long long
decode(FILE *fin, FILE *fout)
{
    static const unsigned char val[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
        0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };
    static unsigned char in[LINES_PER_BLOCK][GROUPS_PER_LINE * 4 + 1];
    static unsigned char out[LINES_PER_BLOCK][GROUPS_PER_LINE * 3];

    /* Initialize input block with valid inputs, for validation */
    memset(in, 0x41, sizeof(in));
    for (int i = 0; i < LINES_PER_BLOCK; i++)
        in[i][sizeof(in[0]) - 1] = 0x0a;

    for (unsigned long long nblocks = 0; ; nblocks++) {
        size_t out_len;
        size_t in_len = fread(in, 1, sizeof(in), fin);
        if (!in_len) {
            break;
        } else if (in_len < sizeof(in)) {
            /* This is a partial block. Compute the output size and leave
             * error detection to block processing.
             */
            int nlines = (in_len + sizeof(in[0]) - 1) / sizeof(in[0]);
            int ntail = in_len - (nlines - 1) * sizeof(in[0]);
            unsigned char *end = in[nlines - 1] + ntail;
            if (ntail < 5 || ntail % 4 != 1) {
                /* Invalid input. Damage it and let block processing catch
                 * the error.
                 */
                end[-1] = 0x0a;
                out_len = 0;
            } else {
                out_len = (nlines - 1) * GROUPS_PER_LINE * 3 + ntail / 4 * 3;
                if (end[-1] != 0x0a) {
                    /* Invalid input. Damage it for later detection. */
                    end[-1] = 0x0a;
                } else if (ntail != sizeof(in[0])) {
                    /* Change newline to zero bits so it's not detected as
                     * an error later.
                     */
                    end[-1] = 0x41;
                }
                if (end[-2] == '=') {
                    out_len--;
                    end[-2] = 0x41;
                }
                if (end[-3] == '=') {
                    out_len--;
                    end[-3] = 0x41;
                }
            }
        } else {
            out_len = sizeof(out);
        }

        /* Convert entire block no matter the input length */
        int n;
        int badline = LINES_PER_BLOCK;
        #pragma omp parallel for
        for (n = 0; n < LINES_PER_BLOCK; n++) {
            unsigned char *pi = in[n];
            unsigned char *po = out[n];
            int check[2];
            check[1] = 0;
            /* Unrolling this inner loop has HUGE performance gains. */
            UNROLL
            for (int i = 0; i < GROUPS_PER_LINE; i++) {
                po[i*3+0] = val[pi[i*4+0]] << 2 | val[pi[i*4+1]] >> 4;
                po[i*3+1] = val[pi[i*4+1]] << 4 | val[pi[i*4+2]] >> 2;
                po[i*3+2] = val[pi[i*4+2]] << 6 | val[pi[i*4+3]] >> 0;
                check[val[pi[i*4+0]] >> 7] = 1;
                check[val[pi[i*4+1]] >> 7] = 1;
                check[val[pi[i*4+2]] >> 7] = 1;
                check[val[pi[i*4+3]] >> 7] = 1;
            }
            if (check[1] || pi[sizeof(in[0]) - 1] != 0x0a) {
                /* Remember this line if it's the earliest error. If this
                 * code is reached, performance no longer matters.
                 */
                #pragma omp critical
                badline = n < badline ? n : badline;
            }
        }

        /* An error was discovered, report the line number. */
        if (badline != LINES_PER_BLOCK) {
            return nblocks * LINES_PER_BLOCK + badline + 1;
        }

        /* Every looks good, write the output. */
        if (!fwrite(out, out_len, 1, fout)) {
            /* Output error */
            break;
        }

        /* Was this the last block? */
        if (out_len < sizeof(out))
            break;
    }

    return 0;
}

static int xoptind = 1;
static int xoptopt;
static char *xoptarg;

/* Same as getopt(3) but never prints errors. */
static int
xgetopt(int argc, char * const argv[], const char *optstring)
{
    static int optpos = 1;
    const char *arg;

    arg = xoptind < argc ? argv[xoptind] : 0;
    if (arg && arg[0] == '-' && arg[1] == '-' && !arg[2]) {
        xoptind++;
        return -1;
    } else if (!arg || arg[0] != '-' || !isalnum(arg[1])) {
        return -1;
    } else {
        const char *opt = strchr(optstring, arg[optpos]);
        xoptopt = arg[optpos];
        if (!opt) {
            return '?';
        } else if (opt[1] == ':') {
            if (arg[optpos + 1]) {
                xoptarg = (char *)arg + optpos + 1;
                xoptind++;
                optpos = 1;
                return xoptopt;
            } else if (argv[xoptind + 1]) {
                xoptarg = (char *)argv[xoptind + 1];
                xoptind += 2;
                optpos = 1;
                return xoptopt;
            } else {
                return ':';
            }
        } else {
            if (!arg[++optpos]) {
                xoptind++;
                optpos = 1;
            }
            return xoptopt;
        }
    }
}

static int
usage(FILE *f)
{
    static const char usage[] =
    "usage: race64 [-dh] [-o OUTFILE] [INFILE]\n"
    "  -d       Decode input (default: encode)\n"
    "  -h       Print this help message\n"
    "  -o FILE  Output to file instead of standard output\n"
    "  -V       Print version information\n";
    return fwrite(usage, sizeof(usage)-1, 1, f) && !fflush(f);
}

static int
version(void)
{
    static const char version[] = "race64 " VERSION "\n";
    return fwrite(version, sizeof(version)-1, 1, stdout) && !fflush(stdout);
}

static const char *
run(int argc, char **argv)
{
    enum {MODE_ENCODE, MODE_DECODE} mode = MODE_ENCODE;
    const char *outfile = 0;
    FILE *in = stdin;
    FILE *out = stdout;
    static char err[256];
    unsigned long long lineno;
    int option;

#ifdef _WIN32
    int _setmode(int, int);
    _setmode(_fileno(stdout), 0x8000);
    _setmode(_fileno(stdin), 0x8000);
#endif

    while ((option = xgetopt(argc, argv, "dho:V")) != -1) {
        switch (option) {
        case 'd': mode = MODE_DECODE;
                  break;
        case 'h': return usage(stdout) ? 0 : "<stdout>";
        case 'o': outfile = xoptarg;
                  break;
        case 'V': return version() ? 0 : "<stdout>";
        case ':': usage(stderr);
                  snprintf(err, sizeof(err), "missing argument: -%c", xoptopt);
                  return err;
        case '?': usage(stderr);
                  snprintf(err, sizeof(err), "illegal option: -%c", xoptopt);
                  return err;
        }
    }

    /* Configure input */
    const char *infile = argv[xoptind];
    if (infile) {
        if (argv[xoptind+1]) {
            return "too many arguments";
        }
        in = fopen(infile, "rb");
        if (!in) {
            return infile;
        }
    }
    setvbuf(in, 0, _IONBF, 0);

    /* Configure output */
    if (outfile) {
        out = fopen(outfile, "wb");
        if (!out) {
            return outfile;
        }
    }
    setvbuf(out, 0, _IONBF, 0);

    switch (mode) {
    case MODE_ENCODE:
        encode(in, out);
        break;
    case MODE_DECODE:
        lineno = decode(in, out);
        if (lineno) {
            snprintf(err, sizeof(err), "%s:%llu:invalid input",
                     outfile ? outfile : "<stdin>", lineno);
            return err;
        }
        break;
    }

    if (ferror(in)) {
        return infile ? infile : "<stdin>";
    }

    fflush(out);
    if (ferror(out)) {
        return outfile ? outfile : "<stdout>";
    }

    return 0;
}

int
main(int argc, char **argv)
{
    const char *err = run(argc, argv);
    if (err) {
        fputs("race64: ", stderr);
        if (errno) {
            fputs(strerror(errno), stderr);
            fputs(": ", stderr);
        }
        fputs(err, stderr);
        fputs("\n", stderr);
        return 1;
    }
    return 0;
}
