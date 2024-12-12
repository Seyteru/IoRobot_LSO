package com.cioffidevivo.client.controller

import com.cioffidevivo.client.model.Message
import com.cioffidevivo.client.model.SocketManager
import com.cioffidevivo.client.view.ChatActivity
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class ChatController(private val chatView: ChatActivity) {
    private val socketManager = SocketManager("172.23.255.255", 8080)
    private val coroutineScope = CoroutineScope(Dispatchers.IO)

    init {
        coroutineScope.launch {
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
                break
            }
        }
    }

    fun closeConnection() {
        socketManager.closeSocket()
    }
}