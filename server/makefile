# Nome del compilatore
CC = gcc

# Flag per il compilatore
CFLAGS = -Wall -Wextra -O2 -I/usr/include/x86_64-linux-gnu
LDFLAGS = -pthread -lcurl -ljson-c # Aggiunto flag per il linker per supportare pthread, curl e json-c

# Nome dell'eseguibile
TARGET = server

# File sorgente
SRCS = connection.c server.c main.c logger.c gpt_client.c

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

# Setup dell'ambiente
setup:
	@echo "Setting up environment..."
	@if [ ! -f .env ]; then cp .env.example .env && echo "Created .env file from .env.example"; fi
	@echo "Please edit .env file and add your OpenAI API key"

# Aggiunge una regola "run" per avviare il server
run: $(TARGET) setup
	./$(TARGET)

# Installazione dipendenze (per sistemi Debian/Ubuntu)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libcurl4-openssl-dev libejson-c-dev

# Dichiarazione di obiettivi "phony" (non sono file)
.PHONY: clean run setup install-deps