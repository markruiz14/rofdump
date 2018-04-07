CFLAGS=-Wall

rofdump: 

clean:
	rm rofdump

debug: rofdump.c 
	$(CC) $< -o rofdump $(CFLAGS) -g
