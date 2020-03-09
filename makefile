aubatch: ./src/aubatch.c ./src/commandline.c ./src/modules.c ./src/microbatch.c
		gcc -o ./aubatch ./src/aubatch.c ./src/commandline.c ./src/modules.c -lpthread -Wall
		gcc -o ./microbatch.out ./src/microbatch.c 

debug: ./src/aubatch.c ./src/commandline.c ./src/modules.c ./src/microbatch.c
		gcc -o ./aubatch -g ./src/aubatch.c ./src/commandline.c ./src/modules.c -lpthread -Wall
		gcc -o ./microbatch.out -g ./src/microbatch.c 
		
