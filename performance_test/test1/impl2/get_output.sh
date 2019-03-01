#!/bin/bash

echo "input size: $1"
echo "num thread: $2"

#for serial program
gcc serial.c -pthread -o serial.out

echo "input: $1 $2" >> serial.txt

for i in {1..5..1}
do
	./serial.out $1 >> ./serial.txt
done	

#for busy wait
echo "input: $1 $2" >> busy_wait.txt
gcc busy_wait.c -pthread -o busy_wait.out

for i in {1..5..1}
do
	./busy_wait.out $1 >> ./busy_wait.txt
done

#for mutex
echo "input: $1 $2" >> mutex.txt
gcc mutex.c -pthread -o mutex.out

for i in {1..5..1}
do
	./mutex.out $1 >> ./mutex.txt
done

#for semaphore
echo "input: $1 $2" >> semaphore.txt
gcc semaphore.c -pthread -o semaphore.out

for i in {1..5..1}
do
	./semaphore.out $1 >> ./semaphore.txt
done
