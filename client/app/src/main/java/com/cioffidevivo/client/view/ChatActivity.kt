package com.cioffidevivo.client.view

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import com.cioffidevivo.client.R
import com.cioffidevivo.client.controller.ChatController
import com.cioffidevivo.client.model.Message

class ChatActivity : AppCompatActivity() {
    private lateinit var furhatMessage: TextView
    private lateinit var etMessage: EditText
    private lateinit var btnSend: Button
    private lateinit var controller: ChatController
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_chat)

        furhatMessage = findViewById(R.id.furhatMessage)
        etMessage = findViewById(R.id.etMessage)
        btnSend = findViewById(R.id.btnSend)

        controller = ChatController(this)

        btnSend.setOnClickListener {
            val message = etMessage.text.toString()
            if(message.isNotBlank()) {
                controller.sendMessage(Message("You", message))
                etMessage.text.clear()
            }
        }
    }

    fun showMessage(message: Message) {
        furhatMessage.append("${message.sender}: ${message.content}")
    }

    fun showError(error: String) {
        furhatMessage.append("Error: $error\n")
    }
}