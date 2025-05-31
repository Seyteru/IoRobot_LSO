import asyncio
import websockets
import socket
import json

FURHAT_URI = "ws://localhost:1932"  # Indirizzo WebSocket del simulatore
LOCAL_TCP_PORT = 5001               # Porta su cui riceviamo dal server C

async def forward_to_furhat(msg):
    try:
        async with websockets.connect(FURHAT_URI) as websocket:
            print(f"[Python] Inviando a Furhat: {msg}")
            await websocket.send(msg)
    except Exception as e:
        print(f"[Python] Errore WebSocket: {e}")

def start_tcp_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(("localhost", LOCAL_TCP_PORT))
    server.listen(1)
    print(f"[Python] Bridge in ascolto su localhost:{LOCAL_TCP_PORT}...")

    while True:
        client, addr = server.accept()
        data = client.recv(1024).decode().strip()
        if data:
            print(f"[Python] Messaggio ricevuto dal C: {data}")
            asyncio.run(forward_to_furhat(data))
        client.close()

if __name__ == "__main__":
    start_tcp_server()