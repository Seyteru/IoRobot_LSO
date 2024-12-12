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

class SocketManager(private val host: String, private val port: Int) {
    private var socket: Socket? = null
    private var writer: BufferedWriter? = null
    private var reader: BufferedReader? = null

    suspend fun connect(): Boolean {
        return withContext(Dispatchers.IO) {
            try {
                socket = Socket(host, port)
                writer = BufferedWriter(OutputStreamWriter(socket?.getOutputStream()))
                reader = BufferedReader(InputStreamReader(socket?.getInputStream()))
                true
            } catch (e: IOException) {
                e.printStackTrace()
                false
            }
        }
    }

    suspend fun sendMessage(message: String): Boolean {
        return withContext(Dispatchers.IO) {
            try {
                writer?.write(message + "\n")
                writer?.flush()
                true
            } catch (e: IOException) {
                e.printStackTrace()
                false
            }
        }
    }

    suspend fun receiveMessage(): String? {
        return withContext(Dispatchers.IO) {
            try {
                reader?.readLine()
            } catch (e: IOException) {
                e.printStackTrace()
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