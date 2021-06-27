CC=gcc
CFLAGS+=-c -g

TARGET=a.out
INCLUDE=../include
SRC=../src
OBJS=main.o client.o parser.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ -lfreetype -lm

%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -o $@ $^ -I $(INCLUDE)  -I /usr/local/include/freetype2 -I /usr/include/libpng16

clean:
	rm -f *.o $(TARGET) catche

.PHONY: all clean
