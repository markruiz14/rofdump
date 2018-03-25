all:
	gcc rofdump.c -o rofdump

debug:
	gcc -g rofdump.c -o rofdump
