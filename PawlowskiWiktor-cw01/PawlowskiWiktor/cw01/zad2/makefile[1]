all:
#cleaning at first
	make clean
#making a static library	
	make static
# -Wall Recommended compiler warnings
# -g generate debugging information
# -o main place output in file main 
# -L. take library from directory .
#-lrary take library library.a
	gcc -Wall -g main.c -o main -L. -lrary

static:
#making a static library
#making .o file
# -c generate only .o file not executable one
	gcc -Wall -c -g library.c
#creating static library
	ar crs library.a library.o

clean:
#removes .o .so and .a files
	rm -f *.o *.a *.so main tmpfile

test:
	make all
	./main

# ar - create, modify, and extract from archives 
# c create s Write an object-file index into the archive, or update an existing one
# r Insert the files member... into archive
