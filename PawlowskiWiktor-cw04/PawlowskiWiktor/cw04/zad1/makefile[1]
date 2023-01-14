all: main.c
	make normal
	make child

normal: main.c
	wersja tylko z jednym procesem
	gcc -Wall -g main.c -o normal

children:
#wersja z dodatkowymi procesami potomnymi
	gcc -Wall -g mainchildren.c -o mainchildren

clean:
	rm normal
	rm mainchildren
