# written by Professor Tuck
BIN  := nush
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

CFLAGS := -g
LDLIBS :=

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

%.o : %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o $(BIN) tmp *.plist valgrind.out main.out

test: $(BIN)
	perl test.pl

valgrind: $(BIN)
	valgrind -q --leak-check=full --log-file=valgrind.out ./$(BIN)

.PHONY: clean test
