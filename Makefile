CC=gcc
CFLAGS=-Wall -march=native -O3
OUTDIR=out
PROG=$(OUTDIR)/fft
DIFF=test/diff.py
TESTFLAGS=
DIFFFLAGS=-t 3e-09

.PHONY: all
all: $(PROG)

.PHONY: clean
clean: | $(OUTDIR)
	rm -f $(OUTDIR)/*
	rmdir $(OUTDIR)

$(OUTDIR):
	mkdir $(OUTDIR)

$(OUTDIR)/dft: dft.c | $(OUTDIR)
	$(CC) $(CFLAGS) $< -lm -o $@

$(OUTDIR)/fft: fft.c | $(OUTDIR)
	$(CC) $(CFLAGS) $< -lm -o $@

.PHONY: test
testcases:=$(wildcard test/*.tc)
test: $(testcases:test/%.tc=$(OUTDIR)/%.tc.diff)

.SECONDARY: $(testcases:test/%.tc=$(OUTDIR)/%.numpy.out)
$(OUTDIR)/%.numpy.out: test/%.tc test/test.py
	test/test.py -i $< -o $@ $(TESTFLAGS)

.SECONDARY: $(testcases:test/%.tc=$(OUTDIR)/%.tc.out)
$(OUTDIR)/%.tc.out: test/%.tc $(PROG)
	$(PROG) -i $< -o $@ $(TESTFLAGS)

$(OUTDIR)/%.tc.diff: $(OUTDIR)/%.tc.out $(OUTDIR)/%.numpy.out
	$(DIFF) $(@:%.tc.diff=%.tc.out) $(@:%.tc.diff=%.numpy.out) $(DIFFFLAGS)

