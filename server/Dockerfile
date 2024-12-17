# Usa un'immagine di base con GCC e Make
FROM gcc:latest

# Imposta la directory di lavoro nel container
WORKDIR /app

# Copia tutti i file del progetto nella directory di lavoro
COPY . .

# Esegue il comando "make" per costruire l'applicazione
RUN make

# Espone la porta su cui il server è in ascolto (ad esempio, 8080)
EXPOSE 8080

# Comando di default per eseguire il server
CMD ["make", "run"]