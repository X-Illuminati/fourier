CC=gcc
CFLAGS=-Wall
OUTDIR=out
PROG=$(OUTDIR)/dft
DIFF=diff
TESTFLAGS=

.PHONY: all
all: $(PROG)

.PHONY: clean
clean:
	rm -f $(OUTDIR)/*
	rmdir $(OUTDIR)

$(OUTDIR):
	mkdir $(OUTDIR)

$(OUTDIR)/dft: dft.c | $(OUTDIR)
	$(CC) $(CFLAGS) $< -lm -o $@

.PHONY: test
testcases:=$(wildcard test/*.tc)
test: $(testcases:test/%.tc=$(OUTDIR)/%.tc.out)

.SECONDARY: $(testcases:test/%.tc=$(OUTDIR)/%.numpy.out)
$(OUTDIR)/%.numpy.out: test/%.tc test/test.py
	test/test.py -i $< -o $@

$(OUTDIR)/%.tc.out: test/%.tc $(PROG) $(OUTDIR)/%.numpy.out
	$(PROG) -i $< -o $@ $(TESTFLAGS)
	$(DIFF) $@ $(@:%.tc.out=%.numpy.out)

