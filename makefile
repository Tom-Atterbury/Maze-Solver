CFLAGS = -I.
DEPS = maze_types.h

all: solve generate render

%.o: %.c $(DEPS)
	$(CC) -c -g -o $@ $< $(CFLAGS)

solve: solve.o
	gcc -o $@ $^ $(CFLAGS) -pthread

generate: generate.o
	gcc -o $@ $^ $(CFLAGS)

render: render.o
	gcc -o $@ $^ $(CFLAGS) -lpng

clean:
	rm -rf *.o solve generate render
