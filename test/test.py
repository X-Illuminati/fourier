#!/bin/python
import argparse
import numpy as np
import sys

#globals
args=argparse.Namespace()

def _v():
    global args
    return args.verbose

def parse_args(inargs) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="This script will read a test case .tc file from stdin and pass it through numpy.fft before printing the output to stdout.",
        )
    parser.add_argument("-i", "--input", default=sys.stdin,
        type=argparse.FileType('r', encoding="ascii"),
        help="specify an input file")
    parser.add_argument("-o", "--output", default=sys.stdout,
        type=argparse.FileType('w'), help="specify an output file")
    parser.add_argument("-v", "--verbose", action="store_true",
        help="extra output for debug")
    return(parser.parse_args(inargs))

def main(inargs) -> int:
    global args
    global stats
    global interrupt

    args=parse_args(inargs)
    if (_v()):
        print(args, file=sys.stderr)

    num_samples=0
    samples=[]

    for line in args.input:
        try:
            if (_v()):
                print(line, end='', file=sys.stderr)
            num_samples=int(line)
            break
        except ValueError:
            continue

    for line in args.input:
        try:
            if (_v()):
                print(line, end='', file=sys.stderr)
            samples.append(float(line))
            if (len(samples) == num_samples):
                break
        except ValueError:
            continue
    if (_v()):
        print("collected samples:", samples, file=sys.stderr)

    if (len(samples) != num_samples):
        print("Format Error: not enough samples received, expected {}".format(num_samples), file=sys.stderr)

    sp = np.fft.fft(samples)
    print("# {} Frequency Bins".format(len(sp)), file=args.output)
    for v in sp:
        # we'll try 14 significant digits for now and see whether it causes
        # numerical errors that impact our comparison
        print("{:.14g}".format(v), file=args.output)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
