OFLAGS = -O3
CC=g++
STD=-std=c++14
CFLAGS= -g -c -W -Wall -Wextra $(STD) -Wno-missing-field-initializers -Wshadow \
				-I../klib\
				$(OFLAGS)
LFLAGS= -g $(STD) $(OFLAGS)

.PHONY:clean 

Objects= main.o

all : $(Objects) program  

program : $(Objects)
	$(CC) $(Std) $(Objects) $(LFLAGS)  -o program

$(Objects): %.o: %.c *.h
	$(CC) $(CFLAGS) $<

dbg: program
	gdb program

run: program
	./program $(test_size)

time: program
	time ./program $(test_size)

cache: program
	rm c*grind* -f
	valgrind --tool=cachegrind ./program $(test_size)

call: program
	rm c*grind* -f
	valgrind --tool=callgrind ./program $(test_size)

inspect: 
	kcachegrind c*grind\.out\.*

clean:
	rm -f *o 
	rm -f program
	rm -f c*grind\.out\.*
