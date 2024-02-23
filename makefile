COMPILER = gcc
CFLAGS = -Wall -pedantic -g
COBJS = helpers.o
EXES = char_count

all: ${EXES}

char_count: char_count.c ${COBJS}
	${COMPILER} ${CFLAGS} char_count.c ${COBJS} -o char_count

%.o: %.c %.h  makefile
	${COMPILER} ${CFLAGS} $< -c 

clean:
	rm -f *~ *.o ${EXES}

run:
	./char_count 4 text_files