package furhatos.app.client.flow.main

import furhatos.app.client.flow.Parent
import furhatos.app.client.network.ServerConnection
import furhatos.app.client.utils.NumberIntent
import furhatos.flow.kotlin.State
import furhatos.flow.kotlin.state
import furhatos.flow.kotlin.furhat
import org.json.JSONObject

val PersonalityAssessment: State = state(Parent) {
    lateinit var server: ServerConnection

    onEntry {
        try {
            server = ServerConnection("127.0.0.1", 5555)
        } catch (e: Exception) {
            furhat.say("Errore di connessione con il server.")
            goto(Idle)
            return@onEntry
        }
        while (true) {
            val json: JSONObject = server.readJson() ?: break

            when (json.getString("type")) {
                "ask" -> {
                    val question = json.getString("question")

                    var valid = false
                    var responseNumber: Int? = null

                    while (!valid) {
                        val response = furhat.askFor<NumberIntent>(question)

                        val number = response?.number
                        println("DEBUG: Riconosciuto: $number")

                        if (number in 1..7) {
                            responseNumber = number
                            valid = true
                        } else {
                            furhat.say("Per favore, rispondi con un numero da uno a sette.")
                        }
                    }

                    server.sendLine(responseNumber.toString())
                }

                "result" -> {
                    val style = json.getJSONObject("personality").getString("style")
                    furhat.say("Hai uno stile $style.")
                }

                "transition" -> {
                    furhat.say(json.getString("message"))
                }

                else -> break
            }
        }

        server.close()
        goto(FollowUpConversation)
    }

}