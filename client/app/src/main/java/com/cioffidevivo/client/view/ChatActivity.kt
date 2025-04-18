package com.cioffidevivo.client.view

import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.cioffidevivo.client.R
import com.cioffidevivo.client.controller.FurhatController
import com.cioffidevivo.client.model.SocketClient
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class ChatActivity : AppCompatActivity() {

    private lateinit var controller: FurhatController
    private val scope = CoroutineScope(Dispatchers.Main)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_chat)

        val status = findViewById<TextView>(R.id.status)
        val btnConnect = findViewById<Button>(R.id.btnConnect)
        val btnSend = findViewById<Button>(R.id.btnSend)
        val btnDisconnect = findViewById<Button>(R.id.btnDisconnect)
        val inputText = findViewById<EditText>(R.id.inputText)

        controller = FurhatController(
            client = SocketClient("10.0.2.2", 8080), // IP/port
            onAsk = { question ->
                speakOnFurhat(question)

                runOnUiThread {
                    inputText.hint = question
                    btnSend.isEnabled = true
                }
            },
            onResult = { resultJson ->
                runOnUiThread {
                    // Mostra il risultato all’utente
                    Toast.makeText(this, "Risultato: $resultJson", Toast.LENGTH_SHORT).show()
                }
            }
        )

        btnConnect.setOnClickListener {
            scope.launch {
                val ok = controller.startSession()
                runOnUiThread {
                    status.text = if (ok) "Connesso al server ✅" else "Connessione fallita ❌"
                }
            }
        }

        btnSend.setOnClickListener {
            val answer = inputText.text.toString()
            if (answer.isNotBlank()) {
                scope.launch {
                    controller.sendAnswer(answer)
                }
                runOnUiThread {
                    btnSend.isEnabled = false
                    inputText.text.clear()
                }
            }
        }

        btnDisconnect.setOnClickListener {
            scope.launch {
                controller.stopSession()
                runOnUiThread { status.text = "Disconnesso" }
            }
        }
    }

    private fun speakOnFurhat(text: String) {
        // Qui puoi riutilizzare il tuo SocketClient/Controller verso Furhat (porta 1932)
        // oppure puoi avere un secondo SocketClient dedicato a Furhat
    }

    override fun onDestroy() {
        super.onDestroy()
        scope.launch {
            controller.stopSession()
        }
    }
}