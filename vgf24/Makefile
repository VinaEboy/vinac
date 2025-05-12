CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = vina.c arquivo.c auxiliares.c lz.c
OBJ = $(SRC:.c=.o)
EXEC = vinac

# Regra all
all: $(EXEC)

# Gerar executável
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ)

# Compilando
vina.o: vina.c arquivo.h auxiliares.h lz.h
	$(CC) $(CFLAGS) -c vina.c -o vina.o

arquivo.o: arquivo.c arquivo.h
	$(CC) $(CFLAGS) -c arquivo.c -o arquivo.o

auxiliares.o: auxiliares.c auxiliares.h
	$(CC) $(CFLAGS) -c auxiliares.c -o auxiliares.o

lz.o: lz.c lz.h
	$(CC) $(CFLAGS) -c lz.c -o lz.o

# Limpando
clean:
	rm -f *.o $(EXEC)

