#!/bin/python
import argparse
import cmath
import sys

#globals
args=argparse.Namespace()

def _v():
    global args
    return args.verbose

def parse_args(inargs) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="This script will read two test case outputs and compare them to 10 (configurable) decimal places",
        )
    parser.add_argument("INPUT1", type=argparse.FileType('r', encoding="ascii"))
    parser.add_argument("INPUT2", type=argparse.FileType('r', encoding="ascii"))
    parser.add_argument("-a", "--all", action="store_true",
        help="don't stop after the first mismatch")
    parser.add_argument("-t", "--tolerance", action="store", default=1e-9,
        type=float, help="specify the absolute tolerance for the comparison (default: %(default)s)")
    parser.add_argument("-v", "--verbose", action="store_true",
        help="extra output for debug")
    return(parser.parse_args(inargs))

def main(inargs) -> int:
    global args
    global stats
    global interrupt
    retval=0
    samples1=[]
    samples2=[]

    args=parse_args(inargs)
    if (_v()):
        print(args, file=sys.stderr)

    for line in args.INPUT1:
        try:
            samples1.append(complex(line))
        except ValueError:
            if (_v()):
                print(line, end='', file=sys.stderr)
            continue

    for line in args.INPUT2:
        try:
            samples2.append(complex(line))
        except ValueError:
            if (_v()):
                print(line, end='', file=sys.stderr)
            continue

    if (_v()):
        print("collected {} samples from input1:".format(len(samples1)), samples1, file=sys.stderr)
        print("collected {} samples from input2:".format(len(samples2)), samples2, file=sys.stderr)

    if (len(samples1) != len(samples2)):
        print("File length mismatch: input1={}, input2={}".format(len(samples1), len(samples2)), file=sys.stderr)
        retval+=1

    if ((0==retval) or args.all):
        for i in range(min(len(samples1), len(samples2))):
            if (not cmath.isclose(samples1[i], samples2[i],
                abs_tol=args.tolerance, rel_tol=0.0)):
                retval+=1
                print("Mismatch in sample {}: input1={}, input2={}".format(i, samples1[i], samples2[i]), file=sys.stderr)
                if (not args.all):
                    break

    return (retval if retval<256 else 255)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
