The .tc files in this directory each represent a simple test case.

Simple format for the file:
- File encoding is standard ASCII text
- End each line with single \r\n for compatibility with less elegant operating
  systems
- First line should describe an integer number of samples for the file
- Subsequent lines each describe a single floating point or integer sample
- The samples need to be written in an encoding that is compatible with both
  python and libc string input processing
- Lines beginning with # are comment lines and are ignored

Each test case will be run through the executable under test and through a
python script that uses numpy.fft as a reference. The results will then be
compared.

