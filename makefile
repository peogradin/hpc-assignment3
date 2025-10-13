BINS = newton
CFLAGS = -O0 -g

.PHONY : all

all : $(BINS)

newton : newton.c
	gcc $(CFLAGS) -o $@ $<

.PHONY : clean

clean : 
	rm -rf $(BINS)
