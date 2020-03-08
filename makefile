aubatch: ./src/aubatch.c ./src/commandline.c ./src/modules.c ./src/microbatch.c
		gcc -o ./src/aubatch.out ./src/aubatch.c ./src/commandline.c ./src/modules.c -lpthread -Wall
		gcc -o ./src/microbatch.out ./src/microbatch.c 

debug: ./src/aubatch.c ./src/commandline.c ./src/modules.c ./src/microbatch.c
		gcc -o ./src/aubatch.out -g ./src/aubatch.c ./src/commandline.c ./src/modules.c -lpthread -Wall
		gcc -o ./src/microbatch.out -g ./src/microbatch.c 
		
