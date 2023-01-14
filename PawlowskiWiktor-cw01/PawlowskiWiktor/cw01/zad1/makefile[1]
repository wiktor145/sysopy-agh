all:
	make static

static:
#by default, creates static library
#cleaning at first
	make clean
#making .o file
	gcc -Wall -c -g library.c
#creating static library
	ar crs library.a library.o

shared:
#cleaning
	make clean
#making .o file
	gcc -Wall -c -fPIC -g library.c
#creating shared library
	gcc -fPIC -Wall -shared -o library.so library.o -Wl,-rpath=.


clean:
#removes .o .so and .a files
	rm -f *.o *.a *.so


# -Wall Recommended compiler warnings
# -g generate debugging information
# -o main place output in file main 
# -c generate only .o file not executable one
# -fpic  Generate position-independent code 
# -shared   generates shared object file for shared library.

# ar - create, modify, and extract from archives 
# c create s Write an object-file index into the archive, or update an existing one
# r Insert the files member... into archive
