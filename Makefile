# you start with default being 'sum'
default: sum

# but your submission will also include 'par_sum'
# default: sum par_sum

sum: sum.c
	gcc -g -O2 --std=c99 -Wall -o sum sum.c

par_sum: par_sum.c
	gcc -g -O2 --std=c99 -Wall -o par_sum par_sum.c -lpthread

clean:
	rm -f sum par_sum
