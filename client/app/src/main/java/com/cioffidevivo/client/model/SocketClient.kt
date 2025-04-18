package com.cioffidevivo.client.model

import android.util.Log
import okio.IOException
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.PrintWriter
import java.net.Socket
import kotlin.concurrent.thread

class SocketClient(private val serverIp: String, private val serverPort: Int) {
    private var socket: Socket? = null
    private var writer: PrintWriter? = null
    private var reader: BufferedReader? = null

    fun connect(): Boolean {
        return try {
            socket = Socket(serverIp, serverPort)
            writer = PrintWriter(socket!!.getOutputStream(), true)
            reader = BufferedReader(InputStreamReader(socket!!.getInputStream()))
            true
        } catch (e: IOException) {
            e.printStackTrace()
            false
        }
    }

    fun sendMessage(message: String) {
        writer?.println(message)
        writer?.flush()
    }

    fun listen(onMessage: (String) -> Unit) {
        thread {
            try {
                var line: String?
                while (true) {
                    line = reader?.readLine()
                    if (line == null) {
                        Log.d("SocketClient", "Connessione chiusa dal server.")
                        break
                    }
                    Log.d("SocketClient", "Messaggio ricevuto: $line") // <-- aggiunto log
                    onMessage(line)
                }
            } catch (e: IOException) {
                Log.e("SocketClient", "Errore durante la lettura del messaggio", e)
            }
        }
    }

    fun disconnect() {
        try {
            writer?.close()
            reader?.close()
            socket?.close()
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }
}