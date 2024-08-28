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

/*** #define values ***/
#define MAX_SAMPLES ((size_t)4096U)
/* TIMING_TEST: number of iterations to repeat the FFT calculation */
//#define TIMING_TEST 20000U

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
uint32_t reverse_bits( uint32_t x )
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
void shuffle(int num_samples, double* const input_buf)
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
}

/* FFT calculation
 * 1. Split the sample into two halves (even/odd fields)
 * 2. Recursively compute the FFT on each half
 * 3. Merge the results
 *
 * Note: transform_buf must already be allocated and can not be NULL
 * Note: modifies input_buf
 */
void fft(long num_samples, double* const input_buf,
    double complex* const transform_buf)
{
    static size_t depth=0; //for debug purposes we will keep track of call depth

    //Check the inputs; particularly that there are a power of 2 samples
    assert(NULL != input_buf);
    assert(NULL != transform_buf);
    assert(0 < num_samples);
    assert(ispowerof2(num_samples));

    //TODO: make an "inner" function for the rest of the recursion
    if (depth == 0) shuffle(num_samples, input_buf);

    //Base Case: num_samples=1
    if (1 == num_samples) {
        //Simply return the input, X₀=x₀
        transform_buf[0]=CMPLX(input_buf[0], 0);
        verbose("Returning %.16Lf%+.16Lfj at Level %zd\n",  creall(transform_buf[0]), cimagl(transform_buf[0]), depth);
    } else {
        size_t curdepth=depth;
        long half_samples = num_samples/2;
        double complex* temp_buf_out_even = NULL;
        double complex* temp_buf_out_odd = NULL;
        long double complex basis = cexpl(-I*M_PI/half_samples);
        long double complex basis_k = 1;

        if (option_verbose) {
            verbose("Sorted Inputs at Level %zd (%ld samples)\n", depth, num_samples);
            for (size_t i=0; i<num_samples; i++)
                verbose("%.16lf\n", input_buf[i]);
        }

        //Recursively call fft on each half
        temp_buf_out_even = malloc(half_samples*sizeof(*temp_buf_out_even));
        temp_buf_out_odd = malloc(half_samples*sizeof(*temp_buf_out_odd));
        depth=curdepth+1; //increment depth before the call
        fft(half_samples, &input_buf[0], temp_buf_out_even);
        depth=curdepth+1; //and reset it depth after the call
        fft(half_samples, &input_buf[half_samples], temp_buf_out_odd);
        depth=curdepth; //and reset it depth after the call

        //Merge the results
        //Xk = Xk_even + Xk_odd*e^(-ikπ/half_samples)
        //Again, not worrying too much about performance here because we will
        //refactor it later.
        basis_k = 1;
        for (size_t k=0; k<half_samples; k++) {
            double complex xk = temp_buf_out_even[k] +
                basis_k*temp_buf_out_odd[k];
            verbose("%zd,%zd: (%+.16lf%+.16lfj)+(%+.16Lf%+.16Lfj)*(%+.16lf%+.16lfj) = %+.16lf%+.16lfj\n", depth, k, creal(temp_buf_out_even[k]), cimag(temp_buf_out_even[k]), creall(basis_k), cimagl(basis_k), creal(temp_buf_out_odd[k]), cimag(temp_buf_out_odd[k]), creal(xk), cimag(xk));
            basis_k = basis_k * basis;
            transform_buf[k] = xk;
        }
        for (size_t k=0; k<half_samples; k++) {
            double complex xk = temp_buf_out_even[k] +
                basis_k*temp_buf_out_odd[k];
            verbose("%zd,%zd: (%+.16lf%+.16lfj)+(%+.16Lf%+.16Lfj)*(%+.16lf%+.16lfj) = %+.16lf%+.16lfj\n", depth, k+half_samples, creal(temp_buf_out_even[k]), cimag(temp_buf_out_even[k]), creall(basis_k), cimagl(basis_k), creal(temp_buf_out_odd[k]), cimag(temp_buf_out_odd[k]), creal(xk), cimag(xk));
            basis_k = basis_k * basis;
            transform_buf[k+half_samples] = xk;
        }

        //Clean up
        if (NULL != temp_buf_out_even)
            free(temp_buf_out_even);
        if (NULL != temp_buf_out_odd)
            free(temp_buf_out_odd);
    }
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

    if (0 == retval) {
        long num_samples = 0;
        double* input_buf = NULL; //note: free when going out of scope
        double complex* transform_buf = NULL; //note: malloc in this function

        // redirect input and output
        if (NULL != option_input_file)
            freopen(option_input_file, "r", stdin);

        if (NULL != option_output_file)
            freopen(option_output_file, "w", stdout);

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
