package com.cioffidevivo.client.model

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.IOException
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.Socket
import java.net.UnknownHostException

class SocketManager(private val host: String, private val port: Int) {
    private var socket: Socket? = null
    private var writer: BufferedWriter? = null
    private var reader: BufferedReader? = null

    suspend fun connect(): Boolean {
        return withContext(Dispatchers.IO) {
            try {
                Log.d("Socketmanager", "Creando socket con host: $host, porta: $port")
                socket = Socket(host, port)
                Log.d("Socketmanager", "Socket Creato!")
                writer = BufferedWriter(OutputStreamWriter(socket?.getOutputStream()))
                reader = BufferedReader(InputStreamReader(socket?.getInputStream()))
                Log.d("SocketManager", "Connessione riuscita!")
                true
            } catch (e: UnknownHostException) {
                Log.e("Socketmanager", "Host non trovato: ${e.message}")
                false
            } catch (e: IOException) {
                Log.e("Socketmanager", "Errore durante la connessione: ${e.message}")
                false
            } catch (e: Exception) {
                Log.e("Socketmanager", "Errore: ${e.message}")
                false
            }
        }
    }

    suspend fun sendMessage(message: String): Boolean {
        return withContext(Dispatchers.IO) {
            try {
                writer?.write(message + "\n")
                writer?.flush()
                Log.d("SocketManager", "Messaggio inviato: $message")
                true
            } catch (e: IOException) {
                Log.e("SocketManager", "Errore nel invio del messaggio!")
                false
            }
        }
    }

    suspend fun receiveMessage(): String? {
        return withContext(Dispatchers.IO) {
            try {
                val message = reader?.readLine()
                Log.d("SocketManager", "Messaggio ricevuto: $message")
                message
            } catch (e: IOException) {
                Log.e("SocketManager", "Errore nel ricevimento del messaggio!")
                null
            }
        }
    }

    fun closeSocket() {
        socket?.close()
        writer?.close()
        reader?.close()
    }
}