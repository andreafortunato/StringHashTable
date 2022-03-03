all:
	gcc -g stringhashtable.c -o stringhashtable -Wall -Wextra
clean:
	-rm stringhashtable