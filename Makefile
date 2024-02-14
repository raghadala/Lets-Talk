
all: s-talk

s-talk: main.o list.o
	gcc -Werror -Wall -g -std=c99 -D _GNU_SOURCE -pthread -o s-talk list.o main.o 

main.o: main.c list.h
	gcc -Werror -Wall -g -std=c99 -D _GNU_SOURCE -pthread -c main.c

clean:    
	rm -f main.o s-talk

