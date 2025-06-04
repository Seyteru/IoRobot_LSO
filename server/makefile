# Nome del compilatore
CC = gcc

# Flag per il compilatore
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -pthread -lcurl # Aggiunto flag per il linker per supportare pthread

# Nome dell'eseguibile
TARGET = server

# File sorgente
SRCS = connection.c server.c main.c logger.c

# File oggetto (derivano dai sorgenti)
OBJS = $(SRCS:.c=.o)

# Regola di default per costruire il target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Regola per compilare i file sorgente in oggetto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia dei file generati
clean:
	rm -f $(OBJS) $(TARGET)

# Aggiunge una regola "run" per avviare il server
run: $(TARGET)
	./$(TARGET)

# Dichiarazione di obiettivi "phony" (non sono file)
.PHONY: clean run