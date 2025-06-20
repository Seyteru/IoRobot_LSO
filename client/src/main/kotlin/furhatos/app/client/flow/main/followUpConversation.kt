package furhatos.app.client.flow.main

import furhatos.app.client.flow.Parent
import furhatos.app.client.flow.server
import furhatos.flow.kotlin.*
import furhatos.gestures.Gestures
import org.json.JSONObject

var currentPersonality: String = ""
var isWaitingForResponse = false
var currentQuestion = ""
var questionCount = 0

val FollowUpConversation: State = state(Parent) {

    onEntry {
        println("DEBUG: Entrando in FollowUpConversation con personalità: $currentPersonality")

        // Richiedi la prima domanda dal server
        requestNextQuestion()
    }

    onResponse {
        if (isWaitingForResponse) {
            val response = it.text.trim()
            println("DEBUG: Follow-up risposta ricevuta: '$response'")

            // Reagisci emotivamente in base alla personalità
            reactToResponse(response)

            // Invia la risposta al server
            server.sendLine(response)
            isWaitingForResponse = false
            questionCount++

            // Richiedi la prossima domanda
            delay(500) // Piccola pausa prima della prossima domanda
            requestNextQuestion()
        }
    }

    onNoResponse {
        if (isWaitingForResponse) {
            handleNoResponse()
            furhat.listen()
        }
    }
}

private fun TriggerRunner<*>.handleNoResponse() {
    when (currentPersonality.lowercase()) {
        "riservato" -> {
            furhat.gesture(Gestures.Thoughtful)
            val messages = arrayOf(
                "Prenditi il tempo che ti serve per pensare...",
                "Non c'è fretta...",
                "Puoi rispondere quando ti senti pronto..."
            )
            furhat.say(messages.random())
        }
        "aperto" -> {
            furhat.gesture(Gestures.BigSmile)
            furhat.attend(users.current)
            val messages = arrayOf(
                "Dai, dimmi quello che pensi! Non essere timido!",
                "Su, raccontami!",
                "Sono curioso di sentire la tua opinione!"
            )
            furhat.say(messages.random())
        }
        else -> {
            furhat.say("Puoi ripetere la tua risposta?")
        }
    }
}

private fun TriggerRunner<*>.reactToResponse(response: String) {
    when (currentPersonality.lowercase()) {
        "riservato" -> {
            // Reazioni più contenute e incerte
            if (response.length > 20) {
                furhat.gesture(Gestures.Nod)
                val reactions = arrayOf(
                    "Ah... interessante...",
                    "Mmm, capisco...",
                    "Sì, va bene..."
                )
                furhat.say(reactions.random())
            } else {
                furhat.gesture(Gestures.Thoughtful)
                val reactions = arrayOf(
                    "Capisco.",
                    "Ah, okay...",
                    "Sì..."
                )
                furhat.say(reactions.random())
            }
        }

        "aperto" -> {
            // Reazioni più enthusiastiche e dirette
            furhat.attend(users.current)
            if (response.length > 20) {
                furhat.gesture(Gestures.BigSmile)
                val reactions = arrayOf(
                    "Che bello!",
                    "Davvero interessante!",
                    "Fantastico! Mi piace il tuo modo di pensare!"
                )
                furhat.say(reactions.random())
            } else {
                furhat.gesture(Gestures.Smile)
                val reactions = arrayOf(
                    "Perfetto!",
                    "Fantastico!",
                    "Ottimo!"
                )
                furhat.say(reactions.random())
            }
        }

        else -> {
            // Comportamento neutro
            furhat.gesture(Gestures.Nod)
            furhat.say("Interessante.")
        }
    }
}

private fun FlowControlRunner.requestNextQuestion() {
    try {
        // Leggi il prossimo messaggio dal server
        val json: JSONObject? = server.readJson()

        if (json != null) {
            when (json.getString("type")) {
                "ask", "gpt_ask" -> {
                    currentQuestion = json.getString("question")

                    // Aggiorna personalità se presente
                    if (json.has("style")) {
                        currentPersonality = json.getString("style")
                    }

                    println("DEBUG: Ricevuta domanda: $currentQuestion")
                    askQuestionWithPersonality(currentQuestion)
                }

                "behavior" -> {
                    // Gestisci comportamenti speciali (pause, etc.)
                    if (json.getString("action") == "pause") {
                        val duration = json.optInt("duration", 1000)
                        Thread.sleep(duration.toLong())
                        requestNextQuestion() // Richiedi la prossima dopo la pausa
                    }
                }

                "reaction" -> {
                    // Il server invia una reazione specifica
                    val message = json.getString("message")
                    val style = json.optString("style", "neutral")

                    when (style) {
                        "hesitant" -> {
                            furhat.gesture(Gestures.Thoughtful)
                            furhat.say(message)
                        }
                        "enthusiastic" -> {
                            furhat.gesture(Gestures.BigSmile)
                            furhat.attend(users.current)
                            furhat.say(message)
                        }
                        else -> {
                            furhat.gesture(Gestures.Nod)
                            furhat.say(message)
                        }
                    }
                    requestNextQuestion()
                }

                "closing", "gpt_closing" -> {
                    val message = json.getString("message")
                    endConversationWithPersonality(message)
                    return
                }

                else -> {
                    println("DEBUG: Tipo messaggio sconosciuto: ${json.getString("type")}")
                    endConversation()
                }
            }
        } else {
            // Nessun messaggio dal server, termina conversazione
            println("DEBUG: Nessun messaggio dal server, terminando conversazione")
            endConversation()
        }

    } catch (e: Exception) {
        println("ERROR: Errore nella comunicazione con il server: ${e.message}")
        endConversation()
    }
}

private fun FlowControlRunner.endConversationWithPersonality(message: String) {
    when (currentPersonality.lowercase()) {
        "riservato" -> {
            furhat.gesture(Gestures.Thoughtful)
            // Evita contatto visivo diretto
            furhat.say(message)
            delay(500)
            furhat.say("Spero di averti messo a tuo agio.")
        }

        "aperto" -> {
            furhat.gesture(Gestures.BigSmile)
            furhat.attend(users.current) // Contatto visivo diretto
            furhat.say(message)
            furhat.gesture(Gestures.Nod)
            furhat.say("È stato davvero divertente parlare con te!")
        }

        else -> {
            furhat.gesture(Gestures.Smile)
            furhat.say(message)
            furhat.say("Arrivederci!")
        }
    }

    goto(Idle)
}

private fun FlowControlRunner.endConversation() {
    val defaultMessage = when (currentPersonality.lowercase()) {
        "riservato" -> "Eh... grazie per aver parlato con me."
        "aperto" -> "È stato fantastico parlare con te! Grazie mille!"
        else -> "Grazie per la conversazione."
    }

    endConversationWithPersonality(defaultMessage)
}

private fun FlowControlRunner.askQuestionWithPersonality(question: String) {
    isWaitingForResponse = true

    when (currentPersonality.lowercase()) {
        "riservato" -> {
            // Comportamento riservato: evita contatto visivo, pause riflessive
            furhat.gesture(Gestures.Thoughtful)

            if (currentPersonality == "riservato") {
                // Pausa aggiuntiva per personalità riservata
                delay((800 + (0..700).random()).toLong()) // Pause casuali 0.8-1.5 secondi
            }

            furhat.say(question, async = false)
        }

        "aperto" -> {
            // Comportamento aperto: contatto visivo diretto, gesticolare
            furhat.gesture(Gestures.BigSmile)
            furhat.attend(users.current ?: users.random)
            furhat.say(question, async = false)
            furhat.gesture(Gestures.Nod) // Incoraggia con un cenno
        }

        else -> {
            // Comportamento neutro
            furhat.gesture(Gestures.Smile)
            furhat.attend(users.current ?: users.random)
            furhat.say(question, async = false)
        }
    }

    furhat.listen()
}