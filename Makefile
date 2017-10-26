
CFLAGS ?= -Wall -Wextra -std=gnu11 -pedantic -O0 -ggdb -DDEBUG -fstack-protector-all -fstack-check

all: sr

clean:
	rm -f sr

run: sr
	./sr srv
