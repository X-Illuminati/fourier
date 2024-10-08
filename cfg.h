/* Feature Configuration Macros for dft.c and fft.c */
#ifndef DFT_CFG_H
#define DFT_CFG_H

/*
 * MAX_SAMPLES defines the maximum number of input samples (and therefore DFT
 * bins). This is a practical limit to avoid problems with corrupted inputs and,
 * in the case of precomputed twiddle factors, identifies the limits to which
 * the algorithm is able to operate.
 */
#define MAX_SAMPLES ((size_t)4096U)

/*
 * TIMING_TEST: number of iterations to repeat the DFT calculation
 * The same input buffer is supplied to the DFT or FFT algorithm in repeated
 * succession up to this many times.
 * The input is only read once and the final output is only emitted after the
 * repeated processing has completed.
 * Leave undefined or 0 for normal processing.
 */
//#define TIMING_TEST 20000U

/*
 * FEATURE_NONRECURSIVE: compute the FFT iteratively
 * Rather than using the recursive implementation (which is preserved for
 * debugging or instructive purposes), uses an iterative implementation that
 * runs in-place on the output buffer.
 * Only applicable to fft.c.
 */
#define FEATURE_NONRECURSIVE

/*
 * FEATURE_PRECOMPUTED_TWIDDLE_FACTORS: use precomputed twiddle factors
 * Rather than computing the complex exponential factors in the course of
 * executing the algorithm, look them up from the precomputed tables in
 * twiddle.h.
 * Only applicable to fft.c.
 */
#define FEATURE_PRECOMPUTED_TWIDDLE_FACTORS

#endif /* DFT_CFG_H */

