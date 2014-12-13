sudo_test: sudo.c
	gcc -o sudo_test sudo.c -g

clean:
	rm sudo_test
