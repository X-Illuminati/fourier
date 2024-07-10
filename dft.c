#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*** #define values ***/
#define MAX_SAMPLES ((size_t)4096U)

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
/* print basic usage information for the program */
void print_help(void)
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
                print_help();
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
                print_help();
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

/* read a single float from stdin into "out", return false on error */
bool read_one_float(float* out)
{
    bool retval = false;
    char* lineptr = NULL;

    while(true) {
        float res;
        char* endptr;

        lineptr = read_input_line();

        if (NULL == lineptr) //EOL or error
/*BREAK*/   break;

        endptr = lineptr;
        res = strtof(lineptr, &endptr);
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
long parse_input(float** input_buf)
{
    long retval = -1;
    long num_samples = 0;
    size_t i;

    if (read_one_long(&num_samples)) {
        verbose("num_samples = %ld\n", num_samples);
        if ((num_samples > 0) && (num_samples < MAX_SAMPLES)) {
            *input_buf = malloc(num_samples * sizeof(float));
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
            if (read_one_float(&((*input_buf)[i]))) {
                verbose("%ld: %.14g\n", i, (*input_buf)[i]);
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

/* main logic */
int main(int argc, char* const argv[])
{
    int retval = 0;

    // parse arguments
    retval = parse_args(argc, argv);

    if (0 == retval) {
        long num_samples = 0;
        float* input_buf = NULL; //note: free when going out of scope
        float* transform_buf = NULL; //TODO Complex type

        // redirect input and output
        if (NULL != option_input_file)
            freopen(option_input_file, "r", stdin);

        if (NULL != option_output_file)
            freopen(option_output_file, "w", stdout);

        // read samples from input
        num_samples = parse_input(&input_buf);
        if (num_samples <= 0) {
            retval = 2;
        } else {
            // perform DFT processing
            // placeholder
            transform_buf = input_buf;

            if (NULL != transform_buf) {
                size_t i;
                // write output
                printf("# %ld Frequency Bins\n", num_samples);
                for (i=0; i< num_samples; i++)
                    printf("%.14g\n", transform_buf[i]);
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
