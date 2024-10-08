#!/bin/python
import argparse
import cmath
import math
import sys

#globals
args=argparse.Namespace()

def _v():
    global args
    return args.verbose

def powerof2(strint) -> int:
    ival=int(strint)
    if (ival<2) or (ival.bit_count() != 1):
        raise(argparse.ArgumentTypeError("Argument is not a power of 2"))
    return ival

def parse_args(inargs) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="This script generates twiddle.h")
    parser.add_argument("-m", "--max", default=4096, type=powerof2,
        help="specify the maximum FFT size (must be a power of 2)")
    parser.add_argument("-o", "--output", default="twiddle.h",
        type=argparse.FileType('w'), help="specify an different output file")
    parser.add_argument("-v", "--verbose", action="store_true",
        help="extra output for debug")
    return(parser.parse_args(inargs))

# translate numbers into subscript form
def subscript(val) -> str:
    tln = str.maketrans("0123456789", "₀₁₂₃₄₅₆₇₈₉")
    return str(val).translate(tln)

# TBD: use this to clamp numbers that are close to 0 or 1
def clamp(cval: complex) -> complex:
    return cval

def main(inargs) -> int:
    global args
    global stats
    global interrupt

    args=parse_args(inargs)
    if (_v()):
        print(args, file=sys.stderr)

    print("""/* Precomputed Twiddle Factors for FFT up to {} bins */
/* NOTE: THIS IS A GENERATED FILE, DO NOT EDIT */
#ifndef FFT_TWIDDLE_H
#define FFT_TWIDDLE_H

#include <complex.h>
""".format(args.max), file=args.output)

    i=2
    i2=1 # i//2
    count=0
    while (i <= args.max):
        print("const double complex W{}²[{}] =\n{{".format(subscript(i), i2),
            file=args.output)

        for k in range(i//2):
            print("\t{},".format(clamp(cmath.exp(-1j*k*math.pi/i2))),
                file=args.output)

        print("};\n", file=args.output)
        i=i*2
        i2=i2*2
        count=count+1

    print("const double complex* const W²[{}] =\n{{".format(count),
            file=args.output)
    i=2
    while (i <= args.max):
        print("\tW{}²,".format(subscript(i)), file=args.output)
        i=i*2
    print("};\n", file=args.output)

    print("#endif /* FFT_TWIDDLE_H */", file=args.output)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
