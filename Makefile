CFLAGS=-Wall
CC=gcc

rofdump: rofdump.c 
	$(CC) $< -o $@ $(CFLAGS) 

clean:
	rm rofdump

debug: rofdump.c 
	$(CC) $< -o rofdump $(CFLAGS) -g
