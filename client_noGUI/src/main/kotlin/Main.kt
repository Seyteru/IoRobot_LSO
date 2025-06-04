package org.cioffiDeVivo

import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import java.io.*
import java.net.Socket
import java.net.HttpURLConnection
import java.net.URL
import org.json.JSONObject
import java.net.URI

fun main() {

    val client = OkHttpClient()
    val furhatSocket = run {
        val request = Request.Builder()
            .url("ws://localhost:1932")
            .build()

        client.newWebSocket(request, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                println("🟢 Connesso a Furhat Remote API via WebSocket")
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                println("❌ Errore WebSocket Furhat: ${t.message}")
            }

            override fun onMessage(webSocket: WebSocket, text: String) {
                println("📨 Risposta da Furhat: $text")
            }
        })
    }

    // Connessione al server C
    val socket = Socket("127.0.0.1", 5555)
    val input = BufferedReader(InputStreamReader(socket.getInputStream()))
    val output = PrintWriter(socket.getOutputStream(), true)

    println("✅ Connesso al server!")

    while (true) {
        val msg = input.readLine() ?: break
        println("📩 Messaggio ricevuto: $msg")

        val json = JSONObject(msg)
        val type = json.getString("type")

        when (type) {
            "ask" -> {
                val question = json.getString("question")
                println("🤖 Furhat chiede: $question")

                // Invio al simulatore Furhat
                val furhatMessage = JSONObject()
                furhatMessage.put("type", "say")
                furhatMessage.put("text", question)
                furhatSocket.send(furhatMessage.toString())

                print("🙋 Risposta utente: ")
                val userInput = readLine() ?: ""

                val responseJson = JSONObject()
                responseJson.put("type", "answer")
                responseJson.put("answer", userInput)

                output.println(responseJson.toString())
            }

            "result" -> {
                val personality = json.getJSONObject("personality")
                val extroversion = personality.getDouble("extroversion")
                val friendliness = if (personality.has("friendliness")) personality.getDouble("friendliness") else -1.0

                println("Estroversione: $extroversion")
                if (friendliness >= 0)
                    println("Amichevolezza: $friendliness")
                else
                    println("Amichevolezza non disponibile.")

                println("🧠 Personalità stimata:")
                println("   - Estroversione: $extroversion")
                println("   - Cordialità: $friendliness")

                break
            }

            else -> {
                println("⚠️ Tipo di messaggio sconosciuto: $type")
            }
        }
    }

    socket.close()
    println("❌ Disconnesso dal server.")
}
