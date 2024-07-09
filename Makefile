CC=gcc
CFLAGS=-Wall

.PHONY: all
all: out/dft

.PHONY: clean
clean:
	rm -f out/*
	rmdir out

.PHONY: test
test:

out:
	mkdir out

out/dft: dft.c | out
	$(CC) $(CFLAGS) $< -o $@

