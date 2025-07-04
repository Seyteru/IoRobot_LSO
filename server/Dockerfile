# Usa un'immagine di base con GCC e Make
FROM gcc:latest

# Installa le dipendenze necessarie
RUN apt-get update && apt-get install -y \
    libcurl4-openssl-dev \
    libjson-c-dev \
    && rm -rf /var/lib/apt/lists/*

# Imposta la directory di lavoro nel container
WORKDIR /app

# Copia prima il Makefile per sfruttare il Docker cache
COPY makefile .

# Copia i file sorgente e header
COPY *.c *.h ./

# Copia altri file necessari (se esistono)
COPY gpt_prompt.txt ./
COPY .env ./

# Compila l'applicazione
RUN make

# Espone la porta su cui il server è in ascolto
EXPOSE 5555

# Comando di default per eseguire il server direttamente
CMD ["./server"]