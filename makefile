aubatch: aubatch.c 
	    gcc -o aubatch.out aubatch.c -lpthread -Wall

debug: aubatch.c
		gcc -o aubatch.out -g aubatch.c -lpthread -Wall