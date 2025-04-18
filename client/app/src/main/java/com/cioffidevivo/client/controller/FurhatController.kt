package com.cioffidevivo.client.controller

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
            client.listen { msgJson ->
                when {
                    msgJson.contains("\"type\":\"ask\"") -> {
                        val question = msgJson.substringAfter("\"question\":\"").substringBefore("\"")
                        onAsk(question)
                    }
                    msgJson.contains("\"type\":\"result\"") -> {
                        onResult(msgJson)
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