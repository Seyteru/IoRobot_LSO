package com.cioffidevivo.client.controller

import android.util.Log
import com.cioffidevivo.client.model.SocketClient
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class FurhatController(
    private val client: SocketClient,
    val onAsk: (String) -> Unit,
    val onResult: (String) -> Unit
) {

    suspend fun startSession(): Boolean = withContext(Dispatchers.IO) {
        val ok = client.connect()
        if (ok) {
            client.listen { msg ->
                Log.d("SOCKET_MESSAGE", "Ricevuto: $msg")
                when {
                    msg.contains("\"type\":\"ask\"") -> {
                        val question = msg.substringAfter("\"question\":\"").substringBefore("\"")
                        onAsk(question)
                    }

                    msg.contains("Sent ask") -> {
                        // esempio: "Sent ask #1: Come ti chiami?"
                        val question = msg.substringAfter(":").trim()
                        onAsk(question)
                    }

                    msg.contains("\"type\":\"result\"") -> {
                        onResult(msg)
                    }
                }
            }
        }
        ok
    }

    suspend fun sendAnswer(answer: String) = withContext(Dispatchers.IO) {
        val json = """
            {
              "type":"answer",
              "answer":"$answer"
            }
        """.trimIndent()
        client.sendMessage(json)
    }

    suspend fun stopSession() = withContext(Dispatchers.IO) {
        client.disconnect()
    }

}