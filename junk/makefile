exec: c-libs
	gcc -g -m32 -o start myshell.o LineParser.o
	rm -f myshell.o LineParser.o
	
c-libs: myshell.c LineParser.c
	gcc -g -m32 -c -o myshell.o myshell.c
	gcc -g -m32 -c -o LineParser.o LineParser.c
	
.PHONY: clean
clean:
	rm -rf ./*.o start
