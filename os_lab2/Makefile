Main: main.cpp colorPrint.asm
	nasm -f elf32 colorPrint.asm
	g++ -m32 -std=c++11 main.cpp colorPrint.o -o main
	rm -rf colorPrint.o
clean:
	rm -rf main
