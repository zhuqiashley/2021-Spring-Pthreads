# you start with default being 'sum'
default: sumsq

# but your submission will also include 'par_sum'
# default: sumsq par_sumsq

sum: sum.c
	gcc -g -O2 --std=c99 -Wall -o sumsq sumsq.c

par_sum: par_sumsq.c
	gcc -g -O2 --std=c99 -Wall -o par_sumsq par_sumsq.c -lpthread

clean:
	rm -f sumsq par_sumsq
