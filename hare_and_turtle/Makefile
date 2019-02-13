all:
	gcc hnt.c -pthread -o hnt.out
test:
	gcc hnt.c -pthread -o hnt.out
	gcc generate_test.c -o gtest.out
	./hnt.out < ./gtest.out
run:
	gcc hnt.c -pthread -o hnt.out
	./hnt.out
clean:
	rm -f *.out
