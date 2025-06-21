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
                "ask", "gpt_ask" -> {
                    currentQuestion = json.getString("question")
                    isWaitingForAnswer = true
                    println("DEBUG: Ricevuta domanda: $currentQuestion")
                    furhat.ask(currentQuestion)
                    return@onEntry // Esci e aspetta la risposta
                }

                "result" -> {
                    val style = json.getJSONObject("personality").getString("style")
                    furhat.say("Hai uno stile $style.")
                    // Salva la personalità per il prossimo stato
                    currentPersonality = style
                }

                "transition" -> {
                    furhat.say(json.getString("message"))
                    // NON chiamare goto qui - continua a processare messaggi
                }

                "state_change" -> {
                    // Il server ci sta dicendo di cambiare stato
                    println("DEBUG: Ricevuto messaggio state_change: ${json.toString()}")
                    if (json.getString("new_state") == "follow_up") {
                        val personality = json.getString("personality")
                        currentPersonality = personality
                        println("DEBUG: Ricevuto segnale di cambio stato, personalità: $personality")

                        // Invia conferma al server
                        println("DEBUG: Invio conferma READY_FOR_FOLLOWUP al server")
                        server.sendLine("READY_FOR_FOLLOWUP")
                        println("DEBUG: Conferma inviata, cambio stato a FollowUpConversation")

                        // Cambia stato
                        goto(FollowUpConversation)
                        return@onEntry
                    }
                }

                else -> {
                    println("DEBUG: Tipo di messaggio non riconosciuto: ${json.getString("type")}")
                    break
                }
            }
        }

        // Nessun altro messaggio dal server, vai al follow-up
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
                server.sendLine(number.toString())
                isWaitingForAnswer = false

                // Continua a leggere messaggi dal server fino al cambio di stato
                while (true) {
                    val json: JSONObject = server.readJson() ?: break

                    when (json.getString("type")) {
                        "ask", "gpt_ask" -> {
                            currentQuestion = json.getString("question")
                            isWaitingForAnswer = true
                            println("DEBUG: Ricevuta nuova domanda: $currentQuestion")
                            furhat.ask(currentQuestion)
                            return@onResponse
                        }

                        "result" -> {
                            val style = json.getJSONObject("personality").getString("style")
                            furhat.say("Hai uno stile $style.")
                            currentPersonality = style
                        }

                        "transition" -> {
                            furhat.say(json.getString("message"))
                        }

                        "state_change" -> {
                            // Il server ci sta dicendo di cambiare stato
                            println("DEBUG: Ricevuto messaggio state_change in onResponse: ${json.toString()}")
                            if (json.getString("new_state") == "follow_up") {
                                val personality = json.getString("personality")
                                currentPersonality = personality
                                println("DEBUG: Ricevuto segnale di cambio stato, personalità: $personality")

                                // Invia conferma al server
                                println("DEBUG: Invio conferma READY_FOR_FOLLOWUP al server")
                                server.sendLine("READY_FOR_FOLLOWUP")
                                println("DEBUG: Conferma inviata, cambio stato a FollowUpConversation")

                                // Cambia stato
                                goto(FollowUpConversation)
                                return@onResponse
                            }
                        }

                        else -> {
                            println("DEBUG: Tipo di messaggio non riconosciuto: ${json.getString("type")}")
                            break
                        }
                    }
                }

                // Se arriviamo qui senza state_change, vai comunque al follow-up
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