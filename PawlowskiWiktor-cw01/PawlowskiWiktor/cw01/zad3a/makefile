all:
#by default make tests
	make test

test:
#this one compiles everything three times
#at first, creates, uses and tests static library, then shared, and dll at last
	make cleanall
	touch results3a.txt
	make static
	echo "###################static library##################" >> results3a.txt
	./main
	make shared
	echo "###################shared library##################" >> results3a.txt
	./main
	make dll
	echo "###################dynamic library##################" >> results3a.txt
	./main
	make clean	
	
static:
#just compiles everything 
	make clean
#making a static library
#making .o file
	gcc -Wall -c -g library.c
#creating static library
	ar crs library.a library.o
#compiling with static library
	gcc -Wall -g main.c -o main -L. -lrary


shared:
#just compiles everything 
	make clean
	gcc -Wall -c -g -shared -fPIC library.c
# -Wl, -rpath=.   add argument rpath=. to linker 
# rpath designates the run-time search path hard-coded in an executable file or library
	gcc -Wall -shared -fPIC -o library.so library.o  -Wl,-rpath=.
	gcc main.c -Wall -o main -L. -lrary -Wl,-rpath=.

dll:
#just compiles everything 
	make clean
	gcc -Wall -c -g -shared -fPIC library.c
	gcc -Wall -shared -fPIC -o library.so library.o  -Wl,-rpath=.
# -ldl add library libdl.so This library includes functions for dynamically loading shared libraries
	gcc main.c -Wall -o main -L. -lrary -Wl,-rpath=. -D DLL -ldl


clean:
#removes everything but results file
	rm -f *.o *.a *.so main tmpfile


cleanall:
#removes .o .so .a main, tmp and results file
	rm -f *.o *.a *.so results3a.txt main tmpfile

#-L<path> option imforms in which directory compiler should search for libarary file
#-l<libraryName> option is about added librarywhich library to add
#-D option could be understood like: compiler add command "#define DLL" to our main.c file

# ar - create, modify, and extract from archives 
# c create s Write an object-file index into the archive, or update an existing one
# r Insert the files member... into archive

# -Wall Recommended compiler warnings
# -g generate debugging information
# -o main place output in file main 
# -L. take library from directory .
#-lrary take library library.a
# -c generate only .o file not executable one

# -fpic  Generate position-independent code 
# -shared   generates shared object file for shared library.


