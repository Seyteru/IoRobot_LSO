package org.cioffiDeVivo

import java.io.*
import java.net.Socket
import java.net.HttpURLConnection
import java.net.URL
import org.json.JSONObject

fun main() {
    val socket = Socket("127.0.0.1", 8080)
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
                val friendliness = personality.getDouble("friendliness")

                println("🧠 Personalità stimata:")
                println("   - Estroversione: $extroversion")
                println("   - Cordialità: $friendliness")

                val behavior = if (extroversion < 0.4) {
                    "Parla con voce calma e evita il contatto visivo."
                } else {
                    "Parla con entusiasmo e mantiene il contatto visivo."
                }

                sendToFurhat("Benvenuto! $behavior")
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

fun sendToFurhat(text: String) {
    val url = URL("http://localhost:12345/api/say")
    val conn = url.openConnection() as HttpURLConnection
    conn.requestMethod = "POST"
    conn.setRequestProperty("Content-Type", "application/json")
    conn.doOutput = true

    val json = """{"text": "$text"}"""

    conn.outputStream.use { os ->
        os.write(json.toByteArray())
    }

    val responseCode = conn.responseCode
    println("📤 Comando inviato a Furhat. Risposta HTTP: $responseCode")
}