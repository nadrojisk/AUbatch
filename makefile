aubatch: ./src/aubatch.c ./src/commandline.c ./src/driver.c ./src/microbatch.c
		gcc -o ./src/driver.out ./src/aubatch.c ./src/commandline.c ./src/driver.c -lpthread -Wall
		gcc -o ./src/microbatch.out ./src/microbatch.c 

debug: ./src/aubatch.c ./src/commandline.c ./src/driver.c ./src/microbatch.c
		gcc -o ./src/driver.out -g ./src/aubatch.c ./src/commandline.c ./src/driver.c -lpthread -Wall
		gcc -o ./src/microbatch.out -g ./src/microbatch.c 
		
