#include <assert.h>
#include <complex.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include "cfg.h"

/*** #define values ***/

/*** global variables ***/
/* option arguments */
bool option_verbose = false;
const char* option_input_file = NULL;
const char* option_output_file = NULL;

/* line pointer for read_input_line helper */
char* ril_lineptr = NULL;

/*** function prototypes ***/

/*** function like macros ***/
#define ispowerof2(unsigned_val) (0 == (unsigned_val & (unsigned_val - 1)))

/* verbose logging */
#define verbose(...) \
    do { \
        if (option_verbose) fprintf(stderr, __VA_ARGS__); \
    } while(0)

/* error logging */
#define error(...) \
    do { \
        fprintf(stderr, __VA_ARGS__); \
    } while(0)

/*** functions definitions ***/
/* print basic usage information for the program
   note: calls exit(exit_code) at end and does not return */
noreturn void print_help(int exit_code)
{
    fprintf(stderr, "\
usage fft [-v] [-h] [-i INPUT] [-o OUTPUT]\n\
\n\
This program will read a test case .tc file from stdin and compute\n\
the Fast Fourier Transform for it and print the result to stdout.\n\
The number of samples in the test case must be a power of 2.\n\
\n\
options:\n\
  -v, --verbose               extra output for debug\n\
  -h, --help                  show this help message and exit\n\
  -i INPUT, --input INPUT     specify an input file\n\
  -o OUPTUT, --output OUTPUT  specify an output file\n\
");

    exit(exit_code);
}

/* parse command line input options and return status */
int parse_args(int argc, char* const argv[])
{
    int retval = 0;
    static const char* optstring = "i:o:hv";
    static struct option long_options[] = {
        {"input",   required_argument, 0, 'i'},
        {"output",  required_argument, 0, 'o'},
        {"help",    no_argument,       0, 'h'},
        {"verbose", no_argument,       0, 'v'},
        {0,         0,                 0, 0}
    };
    int c = 0;

    /* loop until c == -1 indicating end of options help message was printed */
    while (-1 != c) {
        c = getopt_long(argc, argv, optstring, long_options, NULL);
        if (c > 0)
            verbose("read option %c, with optarg %s\n", c, optarg);

        switch(c) {
            case -1:
/*END*/         break;

            case 'v':
                option_verbose = true;
                break;

            case 'h':
/*BREAK*/       c=-1;
/*NORETURN*/    print_help(0);
                break;

            case 'i':
                option_input_file = optarg;
                break;

            case 'o':
                option_output_file = optarg;
                break;

            case '?':
                /* intentional fall-through */
            default:
                retval=1;
/*BREAK*/       c=-1;
/*NORETURN*/    print_help(1);
                break;
        }
    }

    return retval;
}

/* read a single line of input
   returns char* on successfully read, NULL on error */
char* read_input_line(void)
{
    size_t n; //buffer size from getline
    ssize_t g; //retval from getline

    //clean up buffer from previous call
    if (NULL != ril_lineptr) {
        free(ril_lineptr);
        ril_lineptr = NULL;
    }

    //getline not entirely safe... but convenient
    g = getline(&ril_lineptr, &n, stdin);

    if (-1 == g) {
        free(ril_lineptr);
        ril_lineptr = NULL;
    }

    return ril_lineptr;
}

/* read a single long integer from stdin into "out", return false on error */
bool read_one_long(long* out)
{
    bool retval = false;
    char* lineptr = NULL;

    while(true) {
        unsigned long res;
        char* endptr;

        lineptr = read_input_line();

        if (NULL == lineptr) //EOL or error
/*BREAK*/   break;

        endptr = lineptr;
        res = strtoul(lineptr, &endptr, 0);
        if (lineptr != endptr) {
            /* success - this is relatively forgiving pass criteria */
            *out = res;
            retval = true;
/*BREAK*/   break;
        }
    }

    return retval;
}

/* read a single double from stdin into "out", return false on error */
bool read_one_double(double* out)
{
    bool retval = false;
    char* lineptr = NULL;

    while(true) {
        double res;
        char* endptr;

        lineptr = read_input_line();

        if (NULL == lineptr) //EOL or error
/*BREAK*/   break;

        endptr = lineptr;
        res = strtod(lineptr, &endptr);
        if (lineptr != endptr) {
            /* success - this is relatively forgiving pass criteria */
            *out = res;
            retval = true;
/*BREAK*/   break;
        }
    }

    return retval;
}

/* parse testcase data from stdin into input_buf (caller must free)
   and return the number of samples or -1 on error */
long parse_input(double** input_buf)
{
    long retval = -1;
    long num_samples = 0;
    size_t i;

    if (read_one_long(&num_samples)) {
        verbose("num_samples = %ld\n", num_samples);
        if ((num_samples > 0) && (num_samples <= MAX_SAMPLES)) {
            *input_buf = malloc(num_samples * sizeof(**input_buf));
            if (NULL == *input_buf) {
                error("Error allocating %zd bytes for input\n", (num_samples * sizeof(float)));
            } else {
                retval = num_samples;
            }
        } else {
            error("Error: invalid number of samples provided: %ld\n", num_samples);
        }
    } else {
        error("Error parsing testcase number of samples\n");
    }

    if (retval > 0) {
        for (i=0; i<num_samples; i++) {
            if (read_one_double(&((*input_buf)[i]))) {
                verbose("%ld: %.16lf\n", i, (*input_buf)[i]);
            } else {
                error("Error parsing testcase sample %ld\n", i);
                retval = -1;
                free(*input_buf);
                *input_buf = NULL;
/*BREAK*/       break;
            }
        }
    }

    return retval;
}

/* Reverse Bits
 * Returns the integer with the order of bits from x reversed.
 * That is if x=0xA1230000, will return 0x0000C485.
 * Note: this code shamelessly stolen from stackoverflow
 * https://stackoverflow.com/questions/746171
 */
inline uint32_t reverse_bits( uint32_t x )
{
    // Flip pairwise
    x = ( ( x & 0x55555555 ) << 1 ) | ( ( x & 0xAAAAAAAA ) >> 1 );
    // Flip pairs
    x = ( ( x & 0x33333333 ) << 2 ) | ( ( x & 0xCCCCCCCC ) >> 2 );
    // Flip nibbles
    x = ( ( x & 0x0F0F0F0F ) << 4 ) | ( ( x & 0xF0F0F0F0 ) >> 4 );

    // Flip bytes. CPUs have an instruction for that, pretty fast one.
    return __builtin_bswap32( x );
}

/* Bit-Reverse Shuffle
 * Performs the bit-reverse shuffling algorithm in-place on the input data buf.
 * Elements will end up in their final position as though they had been
 * recursively split into even and odd halves.
 *
 * Note: modifies input_buf
 * Note: num_samples must be a power of two
 */
inline void shuffle(long num_samples, double* restrict const input_buf)
{
    double temp;
    int log2samples;
    int half_n = num_samples/2;
    uint32_t i, j;

    assert(0 < num_samples);
    assert(ispowerof2(num_samples));

    // we already know num_samples is a power of 2 so count the zeroes
    log2samples = __builtin_ctz(num_samples);

    // Note: element 0 and N never need to be swapped
    // (they would swap with themselves)
    for (i=1; i<half_n; i++) {
        j = reverse_bits(i)>>(32-log2samples);

        // if i < j, then we haven't swapped these two elements yet
        // if i == j, then we don't need to swap them
        // if i > j, then we have swapped them already and don't need to do it
        // again; however, there will be a matching pair of elements in the top
        // half of the list that still needs to be swapped
        if (i < j) {
            verbose("swapping input %X <-> %X\n", i, j);
            temp = input_buf[j];
            input_buf[j] = input_buf[i];
            input_buf[i] = temp;
        } else if (i > j) {
            verbose("swapping input %X <-> %X\n", i+half_n+1, j+half_n+1);
            temp = input_buf[j+half_n+1];
            input_buf[j+half_n+1] = input_buf[i+half_n+1];
            input_buf[i+half_n+1] = temp;
        }
    }

#ifdef FEATURE_NONRECURSIVE
    if (option_verbose) {
        verbose("Sorted Inputs (%ld samples):\n", num_samples);
        for (size_t i=0; i<num_samples; i++)
            verbose("%.16lf\n", input_buf[i]);
    }
#endif
}

#ifdef FEATURE_NONRECURSIVE
/* Iterative FFT implementation
 * 1. Iterate over the transform_buf in groups of 2, then 4, then 8, etc.
 * 2. Within each group merge the individual elements together
 *
 * Note: no contract checking for performance, don't call directly, call fft()
 * depth parameter is only used for logging
 */
inline void fft_inner(long num_samples,
    double complex* restrict const transform_buf)
{
    size_t g = 2; //grouping
    size_t groups = num_samples/2; //number of groups

    while (g<=num_samples) {
        long half_samples = g/2;
        double complex basis = cexp(-I*M_PI/half_samples);

        for(size_t n=0; n<groups; n++) {
            double complex basis_k = 1;

            //Merge the individual elements in the group
            //Xk = Xk_even + Xk_odd*e^(-ikπ/half_samples)
            //Xj = Xk_even + Xk_odd*e^(-ijπ/half_samples)
            // where j = k+half_samples
            // and, therefore, e^(-ij) = -e^(-ik)
            basis_k = 1;
            for (size_t k=g*n, j=half_samples+g*n; k<half_samples+g*n; k++, j++) {
                double complex xk = transform_buf[k] + basis_k*transform_buf[j];
                double complex xj = transform_buf[k] - basis_k*transform_buf[j];
                verbose("%zd,%zd: (%+.16lf%+.16lfj)+(%+.16lf%+.16lfj)*(%+.16lf%+.16lfj) = %+.16lf%+.16lfj\n", g, k, creal(transform_buf[k]), cimag(transform_buf[k]), creal(basis_k), cimag(basis_k), creal(transform_buf[j]), cimag(transform_buf[j]), creal(xk), cimag(xk));
                verbose("%zd,%zd: (%+.16lf%+.16lfj)-(%+.16lf%+.16lfj)*(%+.16lf%+.16lfj) = %+.16lf%+.16lfj\n", g, j, creal(transform_buf[k]), cimag(transform_buf[k]), creal(basis_k), cimag(basis_k), creal(transform_buf[j]), cimag(transform_buf[j]), creal(xj), cimag(xj));
                basis_k = basis_k * basis;
                transform_buf[k] = xk;
                transform_buf[j] = xj;
            }
        }

        //maintain the helper vars
        g<<=1;
        groups>>=1;
    }
}
#else /* FEATURE_NONRECURSIVE */
/* Recursive FFT implementation
 * 1. Recursively compute the FFT on each half of the input buffer
 * 2. Merge the results
 *
 * Note: no contract checking for performance, don't call directly, call fft()
 * depth parameter is only used for logging
 */
void fft_inner(size_t depth, long num_samples, double* restrict const input_buf,
    double complex* restrict const transform_buf)
{
    //Base Case: num_samples=1
    if (1 == num_samples) {
        //Simply return the input, X₀=x₀
        transform_buf[0]=CMPLX(input_buf[0], 0);
        verbose("Returning %.16lf%+.16lfj at Level %zd\n",  creal(transform_buf[0]), cimag(transform_buf[0]), depth);
    } else {
        long half_samples = num_samples/2;
        double complex basis = cexp(-I*M_PI/half_samples);
        double complex basis_k = 1;

        if (option_verbose) {
            verbose("Sorted Inputs at Level %zd (%ld samples)\n", depth, num_samples);
            for (size_t i=0; i<num_samples; i++)
                verbose("%.16lf\n", input_buf[i]);
        }

        //Recursively call fft on each half
        fft_inner(depth+1, half_samples, &input_buf[0], transform_buf);
        fft_inner(depth+1, half_samples, &input_buf[half_samples],
          &transform_buf[half_samples]);

        //Merge the results
        //Xk = Xk_even + Xk_odd*e^(-ikπ/half_samples)
        //Xj = Xk_even + Xk_odd*e^(-ijπ/half_samples)
        // where j = k+half_samples
        // and, therefore, e^(-ij) = -e^(-ik)
        basis_k = 1;
        for (size_t k=0, j=half_samples; k<half_samples; k++, j++) {
            double complex xk = transform_buf[k] + basis_k*transform_buf[j];
            double complex xj = transform_buf[k] - basis_k*transform_buf[j];
            verbose("%zd,%zd: (%+.16lf%+.16lfj)+(%+.16lf%+.16lfj)*(%+.16lf%+.16lfj) = %+.16lf%+.16lfj\n", depth, k, creal(transform_buf[k]), cimag(transform_buf[k]), creal(basis_k), cimag(basis_k), creal(transform_buf[j]), cimag(transform_buf[j]), creal(xk), cimag(xk));
            verbose("%zd,%zd: (%+.16lf%+.16lfj)-(%+.16lf%+.16lfj)*(%+.16lf%+.16lfj) = %+.16lf%+.16lfj\n", depth, j, creal(transform_buf[k]), cimag(transform_buf[k]), creal(basis_k), cimag(basis_k), creal(transform_buf[j]), cimag(transform_buf[j]), creal(xj), cimag(xj));
            basis_k = basis_k * basis;
            transform_buf[k] = xk;
            transform_buf[j] = xj;
        }
    }
}
#endif /* FEATURE_NONRECURSIVE */

/* FFT calculation
 * 1. Split the sample into two halves (even/odd fields)
 * 2. Call fft_inner() to recursively compute the FFT
 *
 * Note: transform_buf must already be allocated and can not be NULL and
 *       cannot overlap with input_buf
 * Note: modifies input_buf
 */
void fft(long num_samples, double* restrict const input_buf,
    double complex* restrict const transform_buf)
{
    //Check the inputs; particularly that there are a power of 2 samples
    assert(NULL != input_buf);
    assert(NULL != transform_buf);
    assert(0 < num_samples);
    assert(ispowerof2(num_samples));

    // 1. Perform bit-reverse shuffling to split the input buffer into
    //    even and odd samples in O(n) time rather than O(nlog(n)) time.
    shuffle(num_samples, input_buf);

#ifdef FEATURE_NONRECURSIVE
    // 1.5 copy the input_buf to the transform_buf
    for (size_t i=0; i<num_samples; i++)
        transform_buf[i] = CMPLX(input_buf[i], 0);

    // 2. Iteratively compute the FFT
    fft_inner(num_samples, transform_buf);
#else
    // 2. Recursively compute the FFT
    fft_inner(0, num_samples, input_buf, transform_buf);
#endif
}

/* print out the result in the test case output format */
void print_result(long num_bins, const double complex* const bins)
{
    printf("# %ld Frequency Bins\n", num_bins);
    for (size_t i=0; i<num_bins; i++)
        printf("%.16lf%+.16lfj\n", creal(bins[i]), cimag(bins[i]));

    if ((NULL != option_output_file) && (option_verbose)) {
        verbose("# %ld Frequency Bins\n", num_bins);
        for (size_t i=0; i< num_bins; i++)
            verbose("%.16lf%+.16lfj\n", creal(bins[i]), cimag(bins[i]));
    }
}

/* main logic */
int main(int argc, char* const argv[])
{
    int retval = 0;

    // parse arguments
    retval = parse_args(argc, argv);

    // redirect input and output
    if ((0 == retval) && (NULL != option_input_file)) {
        if (NULL == freopen(option_input_file, "r", stdin)) {
          error("Failed to open input file %s\n", option_input_file);
          retval = -1;
        }
    }

    if ((0 == retval) && (NULL != option_output_file)) {
        if (NULL == freopen(option_output_file, "w", stdout)) {
            error("Failed to open output file %s\n", option_output_file);
            retval = -1;
        }
    }

    if (0 == retval) {
        long num_samples = 0;
        double* input_buf = NULL; //note: free when going out of scope
        double complex* transform_buf = NULL; //note: malloc in this function

        // read samples from input
        num_samples = parse_input(&input_buf);
        if ((num_samples <= 0) || (num_samples > 40960)) {
            retval = 2;
        } else {
#if (TIMING_TEST > 0)
            double* input_copy = malloc(num_samples * sizeof(*input_buf));
            memcpy(input_copy, input_buf, (num_samples * sizeof(*input_buf)));
#endif

            transform_buf = malloc(num_samples * sizeof(*transform_buf));

#if (TIMING_TEST > 0)
            for (
              size_t timing_counter = 0;
              timing_counter < (size_t)TIMING_TEST;
              timing_counter++
            ) {
                // replace input with a clean copy
                memcpy(input_buf, input_copy,
                    (num_samples * sizeof(*input_buf)));
#endif
                // peform FFT processing (corrupts input_buf)
                fft(num_samples, input_buf, transform_buf);
#if (TIMING_TEST > 0)
            }
#endif
            if (NULL != transform_buf) {
                // write output
                print_result(num_samples, transform_buf);
                free(transform_buf);
            } else {
                retval = 3;
            }
        }

        if (NULL != input_buf)
            free(input_buf);
    }

    if (NULL != ril_lineptr)
        free(ril_lineptr);

    return retval;
}
