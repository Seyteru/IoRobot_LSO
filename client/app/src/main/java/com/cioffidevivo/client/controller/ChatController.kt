package com.cioffidevivo.client.controller

import android.util.Log
import com.cioffidevivo.client.model.Message
import com.cioffidevivo.client.model.SocketManager
import com.cioffidevivo.client.view.ChatActivity
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class ChatController(private val chatView: ChatActivity) {
    private val socketManager = SocketManager("172.23.255.222", 8080)
    private val coroutineScope = CoroutineScope(Dispatchers.IO)

    init {
        Log.d("ChatController", "Inizializzazione ChatController")
        coroutineScope.launch {
            Log.d("ChatController", "Connessione al server...")
            if(socketManager.connect()) {
                chatView.showMessage(Message("System", "Connected to the Server!"))
                receiveMessages()
            } else {
                chatView.showError("Connection Failure!")
            }
        }
    }

    fun sendMessage(message: Message) {
        coroutineScope.launch {
            val success = socketManager.sendMessage(message.content)
            if(!success) {
                chatView.showError("Error on sending message!")
            } else {
                withContext(Dispatchers.Main) {
                    chatView.showMessage(message)
                }
            }
        }
    }

    private suspend fun receiveMessages() {
        while(true) {
            val message = socketManager.receiveMessage()
            if(message != null) {
                withContext(Dispatchers.Main) {
                    chatView.showMessage(Message("Server", message))
                }
            } else {
                withContext(Dispatchers.Main) {
                    chatView.showError("Connessione persa con il Server!")
                }
                break
            }
        }
    }

    fun closeConnection() {
        socketManager.closeSocket()
    }
}