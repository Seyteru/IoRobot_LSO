package org.cioffiDeVivo

import okhttp3.*
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.RequestBody.Companion.toRequestBody
import java.io.*
import java.net.Socket
import org.json.JSONObject
import java.util.*
import kotlin.system.exitProcess

class FurhatClient {
    private val client = OkHttpClient()
    private val JSON = "application/json; charset=utf-8".toMediaType()
    private var furhatBaseUrl: String? = null
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
    }    fun say(text: String): Boolean {
        if (furhatBaseUrl == null) return false
        
        try {
            val encodedText = java.net.URLEncoder.encode(text, "UTF-8")
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
    }    fun listen(): String? {
        if (furhatBaseUrl == null) return null
        
        try {
            val request = Request.Builder()
                .url("$furhatBaseUrl/furhat/listen?language=en-US")
                .get()
                .build()
            
            val response = client.newCall(request).execute()
            
            if (response.isSuccessful) {
                val responseBody = response.body?.string()
                response.close()
                
                if (responseBody != null) {
                    val jsonResponse = JSONObject(responseBody)
                    if (jsonResponse.has("message")) {
                        return jsonResponse.getString("message")
                    }
                }
            } else {
                response.close()
            }
            
        } catch (e: Exception) {
            // Gestione silenziosa dell'errore
        }
        
        return null
    }
}

fun main() {
    println("🚀 Avvio client IoRobot LSO")
    
    // 1) Connessione al server C
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
    
    // 2) Connessione a Furhat
    val furhatClient = FurhatClient()
    if (!furhatClient.connect()) {
        println("❌ Impossibile connettersi a Furhat Remote API")
        exitProcess(1)
    }
    
    println("✅ Sistema inizializzato correttamente!")
    
    // Loop principale
    try {
        while (true) {
            // Ricevi domanda dal server C
            val questionData = serverInput.readLine() ?: break
            
            // Parse della domanda
            val questionText = try {
                val jsonData = JSONObject(questionData)
                if (jsonData.has("question")) {
                    jsonData.getString("question")
                } else {
                    questionData
                }
            } catch (e: Exception) {
                questionData
            }
            
            // Furhat pronuncia la domanda
            furhatClient.say(questionText)
            
            // Attendi risposta utente
            print("💬 Inserisci la risposta: ")
            val userResponse = readLine()
            
            if (userResponse.isNullOrBlank()) {
                continue
            }
            
            // Invia risposta al server
            serverOutput.println(userResponse)
        }
    } catch (e: Exception) {
        println("❌ Errore nel loop principale: ${e.message}")
    } finally {
        serverSocket.close()
    }
    
    println("✨ Programma terminato")
}
