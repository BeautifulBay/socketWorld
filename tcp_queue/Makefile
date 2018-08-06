
all: list.o queue.o server.o client.o
	@echo "Done!"

%.o:%.c
	gcc -c $< -o $@

client.o : client.c
	gcc $< -o $@

server.o : list.o queue.o server.c
	gcc $^ -o $@ -lpthread

clean:
	rm -rf *.o
