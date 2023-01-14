##arguments are: directory, > or < or =, date in format YYYY-MM-DD HH:MM:SS



all:
	make stat

stat:
	gcc -Wall -g main.c -o program -D STAT

nftw:
	gcc -Wall -g main.c -o program

clean:
	rm program

test:
	gcc -Wall -g main.c -o program -D STAT
	./program "/home/wicia/" ">" "2019-03-18 22:22:22"
