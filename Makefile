pword: tword
	gcc -o pword pword.c

tword: 
	gcc -o tword tword.c

clean:
	rm pword tword
