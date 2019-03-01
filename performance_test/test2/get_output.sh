#!/bin/bash

echo "num operations: $1"
echo "percent insert: $2"
echo "num threads: $3"

#for hash_serial program
gcc hash_serial.c -pthread -o hash_serial.out

echo "input: $1 $2" >> hash_serial.txt

for i in {1..2..1}
do
	./hash_serial.out $1 $2 >> ./hash_serial.txt
done	

#for hash_tablelock
echo "input: $1 $2" >> hash_tablelock.txt
gcc hash_tablelock.c -pthread -o hash_tablelock.out

for i in {1..2..1}
do
	./hash_tablelock.out $1 $2 $3 >> ./hash_tablelock.txt
done

#for hash_rwlock
echo "input: $1 $2" >> hash_rwlock.txt
gcc hash_rwlock.c -pthread -o hash_rwlock.out

for i in {1..2..1}
do
	./hash_rwlock.out $1 $2 $3 >> ./hash_rwlock.txt
done

#for hash_listlock
echo "input: $1 $2" >> hash_listlock.txt
gcc hash_listlock.c -pthread -o hash_listlock.out

for i in {1..2..1}
do
	./hash_listlock.out $1 $2 $3 >> ./hash_listlock.txt
done

#for hash_list_rwlock
echo "input: $1 $2" >> hash_list_rwlock.txt
gcc hash_list_rwlock.c -pthread -o hash_list_rwlock.out

for i in {1..2..1}
do
	./hash_list_rwlock.out $1 $2 $3 >> ./hash_list_rwlock.txt
done


