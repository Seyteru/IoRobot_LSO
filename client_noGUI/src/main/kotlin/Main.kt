package org.cioffiDeVivo

import okhttp3.*
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.RequestBody.Companion.toRequestBody
import java.io.*
import java.net.Socket
import org.json.JSONObject
import java.util.*
import kotlin.system.exitProcess
import kotlin.random.Random

class FurhatClient {
    private val client = OkHttpClient()
    private val JSON = "application/json; charset=utf-8".toMediaType()
    private var furhatBaseUrl: String? = null
    private var currentPersonality: String = "neutro"

    fun connect(): Boolean {
        val furhatHosts = listOf("127.0.0.1", "localhost", "10.5.0.2")

        for (host in furhatHosts) {
            val testUrl = "http://$host:54321"
            try {
                val request = Request.Builder()
                    .url("$testUrl/furhat")
                    .get()
                    .build()

                val response = client.newCall(request).execute()

                if (response.isSuccessful) {
                    println("✅ Connesso a Furhat Remote API")
                    furhatBaseUrl = testUrl
                    response.close()
                    return true
                }
                response.close()
            } catch (e: Exception) {
                // Continua con il prossimo host
            }
        }

        return false
    }

    fun say(text: String, style: String = "neutro"): Boolean {
        if (furhatBaseUrl == null) return false

        try {
            // Aggiungi modificatori al testo in base alla personalità
            val modifiedText = when (style) {
                "riservato" -> addHesitation(text)
                "aperto" -> addEnthusiasm(text)
                else -> text
            }

            val encodedText = java.net.URLEncoder.encode(modifiedText, "UTF-8")
            val request = Request.Builder()
                .url("$furhatBaseUrl/furhat/say?text=$encodedText")
                .post("".toRequestBody(null))
                .build()

            val response = client.newCall(request).execute()
            val success = response.isSuccessful
            response.close()
            return success

        } catch (e: Exception) {
            return false
        }
    }

    private fun addHesitation(text: String): String {
        val hesitations = listOf("Ehm... ", "Allora... ", "Ecco... ", "Beh... ")
        val fillers = listOf("... cioè...", "... diciamo...", "... insomma...")

        return if (Random.nextFloat() < 0.6) {
            val start = if (Random.nextFloat() < 0.4) hesitations.random() else ""
            val end = if (Random.nextFloat() < 0.3) fillers.random() else ""
            "$start$text$end"
        } else {
            text
        }
    }

    private fun addEnthusiasm(text: String): String {
        val enthusiastic = listOf("Fantastico! ", "Che bello! ", "Ottimo! ")

        return if (Random.nextFloat() < 0.4) {
            "${enthusiastic.random()}$text"
        } else if (text.endsWith(".")) {
            text.dropLast(1) + "!"
        } else {
            text
        }
    }

    fun setPersonalityBehavior(personality: String) {
        currentPersonality = personality
        println("🎭 Impostazione personalità: $personality")

        when (personality) {
            "riservato" -> {
                setGaze("away")
                setEmotion("neutral")
                setVoiceRate(0.7)
                setGestures(false)
            }
            "neutro" -> {
                setGaze("straight")
                setEmotion("neutral")
                setVoiceRate(1.0)
                setGestures(false)
            }
            "aperto" -> {
                setGaze("straight")
                setEmotion("happy")
                setVoiceRate(1.2)
                setGestures(true)
            }
        }
    }

    fun performBehavioralAction(action: String, duration: Int = 0) {
        when (action) {
            "pause" -> {
                Thread.sleep(duration.toLong())
            }
            "lookAway" -> {
                setGaze("away")
                Thread.sleep(1500)
                if (currentPersonality != "riservato") {
                    setGaze("straight")
                }
            }
            "showUncertainty" -> {
                setEmotion("confused")
                Thread.sleep(1000)
                setEmotion("neutral")
            }
            "showExcitement" -> {
                setEmotion("happy")
                setGaze("straight")
            }
        }
    }

    fun reactToResponse(style: String) {
        when (style) {
            "hesitant" -> {
                setGaze("down")
                Thread.sleep(500)
                if (currentPersonality == "riservato") {
                    setGaze("away")
                } else {
                    setGaze("straight")
                }
            }
            "enthusiastic" -> {
                setEmotion("happy")
                setGaze("straight")
                // Piccolo movimento della testa
                performHeadNod()
            }
            "neutral" -> {
                setEmotion("neutral")
                setGaze("straight")
            }
        }
    }

    private fun performHeadNod() {
        // Simula un cenno del capo attraverso il gaze
        Thread.sleep(200)
        setGaze("down")
        Thread.sleep(300)
        setGaze("straight")
    }

    fun setGaze(target: String) {
        if (furhatBaseUrl == null) return
        val body = """{"target":"$target"}""".toRequestBody(JSON)
        val request = Request.Builder()
            .url("$furhatBaseUrl/furhat/gaze")
            .post(body)
            .build()
        try {
            client.newCall(request).execute().close()
        } catch (e: Exception) {
            // Gestione silenziosa
        }
    }

    fun setEmotion(emotion: String) {
        if (furhatBaseUrl == null) return
        val body = """{"emotion":"$emotion"}""".toRequestBody(JSON)
        val request = Request.Builder()
            .url("$furhatBaseUrl/furhat/facialExpression")
            .post(body)
            .build()
        try {
            client.newCall(request).execute().close()
        } catch (e: Exception) {
            // Gestione silenziosa
        }
    }

    fun setVoiceRate(rate: Double) {
        if (furhatBaseUrl == null) return
        val body = """{"rate":$rate}""".toRequestBody(JSON)
        val request = Request.Builder()
            .url("$furhatBaseUrl/furhat/voice")
            .post(body)
            .build()
        try {
            client.newCall(request).execute().close()
        } catch (e: Exception) {
            // Gestione silenziosa
        }
    }

    fun setGestures(enabled: Boolean) {
        // Placeholder per il controllo dei gesti
        // Da implementare in base all'API di Furhat disponibile
        println("🤲 Gesti ${if (enabled) "attivati" else "disattivati"}")
    }
}

fun main() {
    println("🚀 Avvio client IoRobot LSO con comportamento adattivo")

    // Connessione al server C
    val serverSocket: Socket
    val serverInput: BufferedReader
    val serverOutput: PrintWriter

    try {
        serverSocket = Socket("localhost", 5555)
        serverInput = BufferedReader(InputStreamReader(serverSocket.getInputStream()))
        serverOutput = PrintWriter(serverSocket.getOutputStream(), true)
        println("✅ Connesso al server C")
    } catch (e: Exception) {
        println("❌ Impossibile connettersi al server C: ${e.message}")
        exitProcess(1)
    }

    // Connessione a Furhat
    val furhatClient = FurhatClient()
    if (!furhatClient.connect()) {
        println("❌ Impossibile connettersi a Furhat Remote API")
        exitProcess(1)
    }

    println("✅ Sistema inizializzato correttamente!")

    var currentPersonality = "neutro"

    // Loop principale
    try {
        while (true) {
            val messageData = serverInput.readLine() ?: break
            val jsonData = try { JSONObject(messageData) } catch (e: Exception) { null }

            when (jsonData?.getString("type")) {
                "result" -> {
                    // Risultato della personalità
                    val personality = jsonData.getJSONObject("personality")
                    currentPersonality = personality.getString("style")

                    println("🧠 Profilo personalità rilevato: $currentPersonality")
                    furhatClient.setPersonalityBehavior(currentPersonality)

                    // Piccola pausa per l'elaborazione
                    Thread.sleep(2000)
                }

                "transition" -> {
                    // Messaggio di transizione
                    val message = jsonData.getString("message")
                    furhatClient.say(message, currentPersonality)
                    Thread.sleep(1500)
                }

                "ask" -> {
                    // Domanda normale
                    val question = jsonData.getString("question")
                    val style = jsonData.optString("style", "neutro")

                    // Comportamento pre-domanda in base alla personalità
                    when (currentPersonality) {
                        "riservato" -> {
                            furhatClient.performBehavioralAction("pause", Random.nextInt(500, 1500))
                            furhatClient.performBehavioralAction("showUncertainty")
                        }
                        "aperto" -> {
                            furhatClient.performBehavioralAction("showExcitement")
                        }
                    }

                    furhatClient.say(question, currentPersonality)

                    // Attendi risposta utente
                    print("💬 Inserisci la risposta: ")
                    val userResponse = readLine()

                    if (!userResponse.isNullOrBlank()) {
                        serverOutput.println(userResponse)
                    }
                }

                "reaction" -> {
                    // Reazione alla risposta dell'utente
                    val message = jsonData.getString("message")
                    val style = jsonData.getString("style")

                    furhatClient.say(message, currentPersonality)
                    furhatClient.reactToResponse(style)
                    Thread.sleep(1000)
                }

                "behavior" -> {
                    // Azione comportamentale specifica
                    val action = jsonData.getString("action")
                    val duration = jsonData.optInt("duration", 1000)
                    furhatClient.performBehavioralAction(action, duration)
                }

                "closing" -> {
                    // Messaggio di chiusura
                    val message = jsonData.getString("message")
                    furhatClient.say(message, currentPersonality)

                    // Comportamento di chiusura personalizzato
                    when (currentPersonality) {
                        "riservato" -> {
                            furhatClient.setGaze("away")
                            Thread.sleep(2000)
                        }
                        "aperto" -> {
                            furhatClient.performHeadNod()
                            furhatClient.setEmotion("happy")
                        }
                    }
                }

                else -> {
                    // Fallback per messaggi non strutturati
                    val questionText = jsonData?.optString("question") ?: messageData
                    furhatClient.say(questionText, currentPersonality)

                    print("💬 Inserisci la risposta: ")
                    val userResponse = readLine()
                    if (!userResponse.isNullOrBlank()) {
                        serverOutput.println(userResponse)
                    }
                }
            }
        }
    } catch (e: Exception) {
        println("❌ Errore nel loop principale: ${e.message}")
    } finally {
        serverSocket.close()
    }

    println("✨ Programma terminato")
}

private fun FurhatClient.performHeadNod() {
    Thread.sleep(200)
    setGaze("down")
    Thread.sleep(300)
    setGaze("straight")
}
