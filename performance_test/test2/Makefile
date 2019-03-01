all:
	gcc serial.c -o serial.out
	gcc busy_wait.c -pthread -o busy_wait.out
	gcc mutex.c -pthread -o mutex.out
	gcc semaphore.c -pthread -o semaphore.out

clean:	
	rm -f *.out
	rm -f *.txt
