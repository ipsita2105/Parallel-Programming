#!/bin/bash

echo "num threads: $1"

#for busywait_barrier program
gcc busywait_barrier.c -pthread -o busywait_barrier.out


for i in {1..5..1}
do
	./busywait_barrier.out $1 >> ./busywait_barrier.txt
done	

#for cond_barrier
gcc cond_barrier.c -pthread -o cond_barrier.out

for i in {1..5..1}
do
	./cond_barrier.out $1 >> ./cond_barrier.txt
done

#for sema_barrier
gcc sema_barrier.c -pthread -o sema_barrier.out

for i in {1..5..1}
do
	./sema_barrier.out $1 >> ./sema_barrier.txt
done

