# fourier
Exploration/Educational Example of DFT/FFT Implementation

## File Listing
* [fft.c](fft.c) - Source file for the FFT implementation
* [cfg.h](cfg.h) - Configuration header for the project
  - see comments in the file for documentation
* [Makefile](Makefile) - input for GNU make
  - supports typical build targets all, clean, and test
* [dft.c](dft.c) - Source file for the DFT implementation
  - kept as a separate file since its implementation is quite different.
  - Edit `PROG` in the [Makefile](Makefile) to build it
  - Or, run: `make PROG=out/dft`
* [twiddle.h](twiddle.h) - header file with precomputed tables for the FFT
  computation
* [twiddle.py](twiddle.py) - python script that generates [twiddle.h](twiddle.h)
* analysis_spreadsheet.ods - spreadsheet to help with analysis
  - 'input tab' displays waveform of pasted input or can help to create an
    input testcase
  - 'output' tab displays spectrum and accepts copy-pasted output from the
    execution
  - 'built-in' tab computes the DFT of the data on the input tab using
    LibreOffice's built-in FOURIER array function, the spectrum is similarly
    plotted for comparison with the 'output' tab
* [test/](test)
  + contains various test cases in .tc files
  + [README.md](test/README.md)
  + [test.py](test/test.py) - python script for testing
    - similar command line and input syntax as the fft executable
    - uses numpy to compute the FFT
    - provides the "expected" results for the unit tests
  + [diff.py](test/diff.py) - python script for comparing results from the fft
    executable and [test.py](test/test.py)
    - accepts a command line option to specify the tolerance for floating point
      comparison

## Building
To build the application:
```sh
make
```

To execute the unit tests:
```sh
make test
```

Output will be in the `out/` directory, you can safely delete it or run:
```sh
make clean
```


## Running
The program takes a testcase in a particular format on standard input and
outputs the complex DFT bins on standard output.  
The format for the testcase input is described in
[test/README.md](test/README.md).

### Examples

#### Basic Example
```sh
out/fft
```
Input:
```
8
1.0
0.0
0.0
0.0
0.0
0.0
0.0
0.0
```
Output:
```
# 8 Frequency Bins
1.0000000000000000+0.0000000000000000j
1.0000000000000000+0.0000000000000000j
1.0000000000000000+0.0000000000000000j
1.0000000000000000+0.0000000000000000j
1.0000000000000000+0.0000000000000000j
1.0000000000000000+0.0000000000000000j
1.0000000000000000+0.0000000000000000j
1.0000000000000000+0.0000000000000000j
```

#### Pipeline Example
```sh
cat test/sawtooth_32_4096.tc | out/fft | plot.py
```

#### Command Line Arguments
```sh
out/fft -h
```
* `-v` / `--verbose` : produce debug output on stderr
* `-i` / `--input` INPUT: read input from file instead of stdin
* `-o` / `--output` OUTPUT: write output to file instead of stdout

Example:
```sh
out/fft -i test/sawtooth_32_4096.tc -o out/output.txt
head out/output.txt
```
Output:
```
# 4096 Frequency Bins
63488.0000000000000000+0.0000000000000000j
0.0000000000000000+0.0000000000000000j
0.0000000000000000+0.0000000000000000j
0.0000000000000000+0.0000000000000000j
0.0000000000000000+0.0000000000000000j
0.0000000000000000+0.0000000000000000j
0.0000000000000000+0.0000000000000000j
0.0000000000000000+0.0000000000000000j
0.0000000000000000+0.0000000000000000j
```

## Additional Notes

### Git History
It is probably worth reviewing the git history if you are examining this
repository from an educational perspective. Each commit builds up the DFT and
then the FFT step-by-step which may aid understanding rather than looking at
the final optimized version.

### Execution Time Evaluation

#### Methodology
A simple approach is used in which the algorithm is executed many times to
gather a statistical view of its average execution time.  
Probably there are other strategies involving performance counters that
could be more precise.

#### Instrumentation
In [cfg.h](cfg.h), uncomment the `TIMING_TEST` #define and chose a value for the
number of iterations to execute. The default value is suitable for a 2017-era
laptop. If you have a recent laptop or a high-performance desktop, you
may want to increase the value. If you have a 32-bit computer or are running
on an ARM CPU and emulating x86-64 instructions, you will surely need to
reduce the number.

#### Test Steps
After modifying [cfg.h](cfg.h), don't forget to run `make`.

At this point, directly using the standard POSIX time command will get you some
basic understanding of the runtime for the selected number of execution cycles
of the test case. Resolution will probably be in tenths of seconds, depending on
your system's implementation of time.

```sh
cat test/sawtooth_32_4096.tc | /usr/bin/time -f %U out/fft >/dev/null
```
Output
```
2.22
```
In this example, 20000 cycles were executed in 1.18 seconds, so the average
execution time was
```math
2.22s / 20000 = 111 μs
```

In addition to specifying repeated executions within the test case, it can help
to run the whole test multiple times to develop a median that removes
outliers. The following command uses
[this repeat.sh script](https://github.com/X-Illuminati/cacography/blob/master/bin/repeat.sh)
([raw](https://raw.githubusercontent.com/X-Illuminati/cacography/refs/heads/master/bin/repeat.sh))
along with
[jq](https://github.com/jqlang/jq)
to create suitable statistics.

```sh
alias stats='jq -s "{ min:min, max:max, sum:add, count:length, avg: (add/length), median: (sort|.[(length/2|floor)]) }"'

repeat.sh -c 10 "cat test/sawtooth_32_4096.tc | /usr/bin/time -f %U out/fft >/dev/null" |& stats
```
Output
```json
{
  "min": 1.18,
  "max": 5.68,
  "sum": 16.809999999999995,
  "count": 10,
  "avg": 1.6809999999999996,
  "median": 1.23
}
```

In this case the median is helpful to throw out the outlier of 5.68 seconds.
Using our median, we get
```math
1.23s / 20000 = 61.5 μs
```

#### Further Reading
The step-by-step approach to DFT and FFT that I followed while creating this
repository came from
> Ken Steiglitz, _A Digital Signal Processing Primer, with Applications to
> Digital Audio and Computer Music_ (1st Edition). Menlo Park, California, USA:
> Addison Wesley, 1995.
[Amazon link to 2nd edition](https://a.co/d/40otwyw)

The Wikipedia page for
[Discrete Fourier transform](https://en.wikipedia.org/wiki/Discrete_Fourier_transform)
may be a useful alternative, but is probably not suitable as an introductory
text.

[DFT matrix](https://en.wikipedia.org/wiki/DFT_matrix) may be more suitable as a
starting point.

With appropriate understanding of DFT, the details of
[Fast Fourier transform](https://en.wikipedia.org/wiki/Fast_Fourier_transform)
and
[Cooley–Tukey FFT algorithm](https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm)
are somewhat more reasonably approachable.