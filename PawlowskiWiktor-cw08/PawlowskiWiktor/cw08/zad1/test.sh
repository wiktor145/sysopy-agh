#!/bin/bash

make 

for c in {3,13,23,33,43,53,65} # wielkosci filtrow
do
	for th in {1,2,4,8} # ilosci watkow
	do
		
		for t in {"block","interleaved"}  #typ dzialania
		do
			./main $th $t "coins.ascii.pgm" "./filtry/filtr$c.txt" "test$c.pgm" "test"
		done
 
	done

done
