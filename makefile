aubatch: ./src/aubatch.c ./src/commandline.c ./src/driver.c
		gcc -o ./src/driver.out ./src/aubatch.c ./src/commandline.c ./src/driver.c -lpthread -Wall

debug: ./src/aubatch.c ./src/commandline.c ./src/driver.c
		gcc -o ./src/driver.out -g ./src/aubatch.c ./src/commandline.c ./src/driver.c -lpthread -Wall

