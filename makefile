OFLAGS = -fno-omit-frame-pointer -O3
CC=g++
STD=-std=c++14
CFLAGS= -g -c -W -Wall -Wextra $(STD) -Wno-missing-field-initializers -Wshadow \
				-I./uthash/include \
				-I./sglib \
				-I/usr/include/glib-2.0 \
				-I/usr/lib/x86_64-linux-gnu/glib-2.0/include\
				-I./klib \
				-I./hhash \
				$(OFLAGS)
LFLAGS= -g $(STD) $(OFLAGS) -lglib-2.0




.PHONY:clean 

Objects= main.o Timer.o

all : $(Objects) program  

program : $(Objects) 
	$(CC) $(Std) $(Objects) $(LFLAGS)  -o program

$(Objects): %.o: %.cpp ./hhash/hop_hash.h
	$(CC) $(CFLAGS) $<

dbg: program
	gdb program

run: program
	./program

time: program
	time ./program

clean:
	rm -f *o 
	rm -f program
	rm -f c*grind\.out\.*
	rm -f [1-9]*00\.dat
