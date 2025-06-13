package furhatos.app.client.flow.main

import furhatos.app.client.flow.Parent
import furhatos.app.client.flow.server
import furhatos.flow.kotlin.State
import furhatos.flow.kotlin.state
import furhatos.flow.kotlin.furhat
import furhatos.flow.kotlin.onNoResponse
import furhatos.flow.kotlin.onResponse
import org.json.JSONObject

val PersonalityAssessment: State = state(Parent) {

    var isWaitingForAnswer = false
    var currentQuestion = ""

    onEntry {
        while (true) {
            val json: JSONObject = server.readJson() ?: break

            when (json.getString("type")) {
                "ask" -> {
                    currentQuestion = json.getString("question")
                    isWaitingForAnswer = true
                    furhat.ask(currentQuestion)
                    return@onEntry // Esci e aspetta la risposta
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

        // Nessun altro messaggio dal server
        goto(FollowUpConversation)
    }

    onResponse {
        if (isWaitingForAnswer) {
            val text = it.text.trim().lowercase()
            println("DEBUG: Ricevuto testo: '$text'")

            val number = when (text) {
                "1", "uno" -> 1
                "2", "due" -> 2
                "3", "tre" -> 3
                "4", "quattro" -> 4
                "5", "cinque" -> 5
                "6", "sei" -> 6
                "7", "sette" -> 7
                else -> {
                    // Fallback: cerca qualsiasi numero da 1 a 7 nel testo
                    val numberRegex = Regex("[1-7]")
                    val match = numberRegex.find(text)
                    match?.value?.toIntOrNull()
                }
            }

            if (number != null) {
                println("DEBUG: Numero riconosciuto: $number")
                furhat.say("Hai detto il numero $number")
                server.sendLine(number.toString())
                isWaitingForAnswer = false

                // Continua con il prossimo messaggio del server
                while (true) {
                    val json: JSONObject = server.readJson() ?: break

                    when (json.getString("type")) {
                        "ask" -> {
                            currentQuestion = json.getString("question")
                            isWaitingForAnswer = true
                            furhat.ask(currentQuestion)
                            return@onResponse
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

                // Nessun altro messaggio dal server
                goto(FollowUpConversation)
            } else {
                println("DEBUG: Numero non riconosciuto")
                furhat.say("Per favore, rispondi con un numero da uno a sette.")
                furhat.listen()
            }
        }
    }

    onNoResponse {
        if (isWaitingForAnswer) {
            furhat.say("Non ti ho sentito. Per favore, ripeti un numero da uno a sette.")
            furhat.listen()
        }
    }

    onReentry {
        if (isWaitingForAnswer) {
            furhat.listen()
        }
    }
}