#include <complex.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
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
usage dft [-v] [-h] [-i INPUT] [-o OUTPUT]\n\
\n\
This program will read a test case .tc file from stdin and compute\n\
the Discrete Fourier Transform for it and print the result to stdout.\n\
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

/* DFT calculation
 * Compute a basis of num_samples equally spaced phasors.
 * Use these to compute the inner product with input_buf and store the
 * results in transform_buf.
 * Note: transform_buf must already be allocated and can not be NULL and
 *       cannot overlap with input_buf
 */
void dft(long num_samples, const double* restrict const input_buf,
    double complex* restrict const transform_buf)
{
    //basis vectors (phasors) are e^(-itk2π/n)
    //here we will compute them incrementally as ((e^(-i2π/n))^k)^t
    //by repeated multiplication basis_k = basis_k * basis
    //and basis_t = basis_t * basis_k
    long double complex basis = cexpl(-I*2*M_PI/num_samples);
    long double complex basis_k = 1;
    size_t k, t;

    verbose("Basis: %.16Lf%+.16Lfj\n", creall(basis), cimagl(basis));

    for (k=0; k<num_samples; k++) {
        long double complex x = 0; //accumulate the calculations for our inner product
        long double complex basis_t = 1;
        verbose("Basis k(%zd): %.16Lf%+.16Lfj\n", k, creall(basis_k), cimagl(basis_k));
        for (t=0; t<num_samples; t++) {
            long double complex xt = 0; //temporary inner product calc
            xt = input_buf[t] * basis_t;
            verbose("x(%zd,%zd) = %+.16lf*(%+.16Lf%+.16Lfj) = %+.16Lf%+.16Lfj\n", k,t, input_buf[t], creall(basis_t), cimagl(basis_t), creall(xt), cimagl(xt));
            basis_t = basis_t * basis_k;
            x += xt;
        }
        verbose("total x                                                    = %+.16Lf%+.16Lfj\n", creall(x), cimagl(x));
        transform_buf[k] = x;
        basis_k = basis_k * basis;
    }
}

/* print out the result in the test case output format */
void print_result(long num_bins, const double complex* const bins)
{
    size_t i;
    printf("# %ld Frequency Bins\n", num_bins);
    for (i=0; i<num_bins; i++)
        printf("%.16lf%+.16lfj\n", creal(bins[i]), cimag(bins[i]));

    if ((NULL != option_output_file) && (option_verbose)) {
        verbose("# %ld Frequency Bins\n", num_bins);
        for (i=0; i< num_bins; i++)
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
            transform_buf = malloc(num_samples * sizeof(*transform_buf));

#if (TIMING_TEST > 0)
            for (
              size_t timing_counter = 0;
              timing_counter < (size_t)TIMING_TEST;
              timing_counter++
            ) {
#endif
              // perform DFT processing
              dft(num_samples, input_buf, transform_buf);
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
