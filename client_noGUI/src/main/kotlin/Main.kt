package org.cioffiDeVivo

import java.io.*
import java.net.Socket
import java.net.HttpURLConnection
import java.net.URL
import org.json.JSONObject

fun main() {
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
