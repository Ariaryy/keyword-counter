CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude

SRC = src/main.c src/lexer.c src/utils.c
OUT = build/counter

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

run: all
	./$(OUT) input.txt

clean:
	rm -rf build