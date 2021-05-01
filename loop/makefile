my_echo: looper.o
	gcc -g -m32 -o looper looper.o
	
looper.o: looper.c
	gcc -g -m32 -c -o looper.o looper.c
	
.PHONY: clean
clean:
	rm -rf ./*.o looper
