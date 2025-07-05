package furhatos.app.client.flow.main

import furhatos.app.client.flow.Parent
import furhatos.app.client.flow.server
import furhatos.flow.kotlin.*
import furhatos.gestures.Gestures
import furhatos.records.Location
import org.json.JSONObject
import kotlin.random.Random

var currentPersonality: String = ""
var isWaitingForResponse = false
var currentQuestion = ""
var questionCount = 0
var conversationEnded = false

// Tracciamento per comportamenti naturali
var lastGestureTime = 0L
var conversationIntensity = 1.0 // Scala da 0.5 a 2.0

val FollowUpConversation: State = state(Parent) {

    onEntry {
        println("DEBUG: Entrando in FollowUpConversation con personalità: $currentPersonality")
        conversationEnded = false
        initializePersonalityBehavior()
        requestNextQuestion()
    }

    onResponse {
        if (isWaitingForResponse) {
            val response = it.text.trim()
            println("DEBUG: Follow-up risposta ricevuta: '$response'")

            // Analizza la risposta per adattare l'intensità
            analyzeResponseIntensity(response)

            // Reagisci emotivamente in base alla personalità
            reactToResponse(response)

            // Invia la risposta al server
            server.sendLine(response)
            isWaitingForResponse = false
            questionCount++

            // Richiedi la prossima domanda
            delay(500)
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

private fun FlowControlRunner.initializePersonalityBehavior() {
    when (currentPersonality.lowercase()) {
        "riservato" -> {
            // Postura iniziale riservata
            furhat.gesture(Gestures.Thoughtful)
            // Evita contatto visivo diretto, guarda leggermente di lato
            furhat.attend(Location(0.3, 0.0, 1.0))
        }

        "aperto" -> {
            // Postura iniziale aperta e accogliente
            furhat.gesture(Gestures.BigSmile)
            furhat.attend(users.current ?: users.random)
            // Gesto di benvenuto
            delay(200)
            furhat.gesture(Gestures.Nod)
        }

        else -> {
            // Neutrale - postura standard
            furhat.gesture(Gestures.Smile)
            furhat.attend(users.current ?: users.random)
        }
    }
}

private fun analyzeResponseIntensity(response: String) {
    // Adatta l'intensità della conversazione basandosi sulla lunghezza e contenuto
    val responseLength = response.length
    val wordsCount = response.split(" ").size

    when {
        responseLength > 50 && wordsCount > 8 -> conversationIntensity = minOf(2.0, conversationIntensity + 0.2)
        responseLength < 10 || wordsCount < 3 -> conversationIntensity = maxOf(0.5, conversationIntensity - 0.1)
        else -> conversationIntensity = (conversationIntensity + 1.0) / 2.0 // Tende verso il neutro
    }
}

private fun TriggerRunner<*>.handleNoResponse() {
    when (currentPersonality.lowercase()) {
        "riservato" -> {
            // Comportamento ancora più riservato quando non c'è risposta
            performReservedWaitingBehavior()
        }

        "aperto" -> {
            // Comportamento incoraggiante ma rispettoso
            performOpenWaitingBehavior()
        }

        else -> {
            performNeutralWaitingBehavior()
        }
    }
}

private fun TriggerRunner<*>.performReservedWaitingBehavior() {
    val waitingBehaviors = arrayOf(
        {
            furhat.gesture(Gestures.Thoughtful)
            // Guarda ancora più lontano, quasi evitando
            furhat.attend(Location(0.5, -0.2, 1.0))
            furhat.say("Um... non c'è fretta...", async = true)
            delay(800)
            // Gesto nervoso - guarda in basso SENZA chiudere occhi
            furhat.attend(Location(0.0, -0.3, 1.0))
            delay(300)
        },
        {
            // Comportamento molto timido
            furhat.attend(Location(0.0, -0.3, 1.0)) // Guarda in basso
            furhat.say("Ehm... puoi pensarci con calma...", async = true)
            delay(1000)
            furhat.gesture(Gestures.Thoughtful)
        },
        {
            // Quasi sussurra
            furhat.gesture(Gestures.GazeAway, async = true)
            delay(200)
            furhat.say("Prenditi... prenditi tutto il tempo che vuoi...", async = true)
            furhat.attend(Location(-0.3, 0.1, 1.0)) // Guarda da un'altra parte
        }
    )
    waitingBehaviors.random().invoke()
}

private fun TriggerRunner<*>.performOpenWaitingBehavior() {
    val waitingBehaviors = arrayOf(
        {
            furhat.gesture(Gestures.BigSmile)
            furhat.attend(users.current ?: users.random)
            furhat.say("Dai, non essere timido! So che hai qualcosa di interessante da dire!", async = true)
            delay(500)
            furhat.gesture(Gestures.Nod)
            // Gesto incoraggiante
            delay(300)
            furhat.gesture(Gestures.Smile)
        },
        {
            // Comportamento giocoso e incoraggiante
            furhat.gesture(Gestures.Surprise)
            furhat.attend(users.current ?: users.random)
            furhat.say("Coraggio! Sono tutto orecchi!", async = true)
            delay(400)
            furhat.gesture(Gestures.BigSmile)
        },
        {
            // Mostra pazienza ma con energia
            furhat.gesture(Gestures.Nod)
            furhat.say("Non preoccuparti, ho tutto il tempo del mondo per te!", async = true)
            delay(600)
            furhat.gesture(Gestures.Smile)
            furhat.attend(users.current ?: users.random) // Mantiene contatto visivo
        }
    )
    waitingBehaviors.random().invoke()
}

private fun TriggerRunner<*>.performNeutralWaitingBehavior() {
    furhat.gesture(Gestures.Thoughtful)
    furhat.say("Puoi ripetere la tua risposta?")
    furhat.attend(users.current ?: users.random)
}

private fun TriggerRunner<*>.reactToResponse(response: String) {
    val responseLength = response.length
    val currentTime = System.currentTimeMillis()

    when (currentPersonality.lowercase()) {
        "riservato" -> reactReserved(response, responseLength)
        "aperto" -> reactOpen(response, responseLength)
        else -> reactNeutral(response, responseLength)
    }

    lastGestureTime = currentTime
}

private fun TriggerRunner<*>.reactReserved(response: String, length: Int) {
    // Comportamenti riservati con sfumature basate sulla lunghezza della risposta
    when {
        length > 50 -> {
            // Risposta lunga - sorpresa timida ma positiva
            furhat.gesture(Gestures.Surprise, async = true)
            delay(300)
            furhat.attend(Location(0.2, 0.1, 1.0)) // Guarda di lato
            val reactions = arrayOf(
                "Oh... questo è... molto interessante...",
                "Wow, non me lo aspettavo... grazie per aver condiviso...",
                "È... è una prospettiva molto ricca..."
            )
            furhat.say(reactions.random(), async = true)
            delay(800)
            furhat.gesture(Gestures.Thoughtful)
        }

        length > 20 -> {
            // Risposta media - apprezzamento timido
            furhat.gesture(Gestures.Nod, async = true)
            furhat.attend(Location(0.1, 0.0, 1.0))
            val reactions = arrayOf(
                "Ah sì... capisco quello che intendi...",
                "Mmm, interessante punto di vista...",
                "Sì... ha senso..."
            )
            furhat.say(reactions.random(), async = true)
            delay(600)
            // Guarda ancora più lontano invece di chiudere occhi
            furhat.attend(Location(0.3, -0.1, 1.0))
            delay(200)
        }

        else -> {
            // Risposta breve - accettazione timida
            furhat.attend(Location(0.0, -0.1, 1.0)) // Guarda leggermente in basso
            val reactions = arrayOf(
                "Capisco...",
                "Ah, okay...",
                "Sì, va bene..."
            )
            furhat.say(reactions.random(), async = true)
            furhat.gesture(Gestures.Thoughtful)
        }
    }
}

private fun TriggerRunner<*>.reactOpen(response: String, length: Int) {
    // Comportamenti aperti ed entusiasti
    when {
        length > 50 -> {
            // Risposta lunga - entusiasmo massimo
            furhat.gesture(Gestures.BigSmile)
            furhat.attend(users.current ?: users.random)
            val reactions = arrayOf(
                "Fantastico! Mi piace davvero come la pensi!",
                "Che bella risposta! Sei proprio in gamba!",
                "Perfetto! È proprio quello che speravo di sentire!"
            )
            furhat.say(reactions.random(), async = true)
            delay(500)
            // Gesti di approvazione multipli
            furhat.gesture(Gestures.Nod, async = true)
            delay(400)
            furhat.gesture(Gestures.Smile)
        }

        length > 20 -> {
            // Risposta media - entusiasmo moderato
            furhat.gesture(Gestures.Smile)
            furhat.attend(users.current ?: users.random)
            val reactions = arrayOf(
                "Che bello! Mi piace!",
                "Davvero interessante!",
                "Ottimo, continua così!"
            )
            furhat.say(reactions.random(), async = true)
            delay(400)
            furhat.gesture(Gestures.Nod)
        }

        else -> {
            // Risposta breve - incoraggiamento positivo
            furhat.gesture(Gestures.Smile)
            furhat.attend(users.current ?: users.random)
            val reactions = arrayOf(
                "Perfetto!",
                "Ottimo!",
                "Fantastico!"
            )
            furhat.say(reactions.random(), async = true)
            // Piccolo gesto di incoraggiamento
            furhat.gesture(Gestures.Nod)
        }
    }
}

private fun TriggerRunner<*>.reactNeutral(response: String, length: Int) {
    // Comportamento neutro bilanciato
    furhat.gesture(Gestures.Nod)
    furhat.attend(users.current ?: users.random)
    val reactions = when {
        length > 30 -> arrayOf("Interessante.", "Capisco.", "Bene.")
        else -> arrayOf("Ok.", "Bene.", "Capito.")
    }
    furhat.say(reactions.random())
}

private fun FlowControlRunner.requestNextQuestion() {
    try {
        val json: JSONObject? = server.readJson()

        if (json != null) {
            when (json.getString("type")) {
                "ask", "gpt_ask" -> {
                    currentQuestion = json.getString("question")

                    if (json.has("style")) {
                        val newPersonality = json.getString("style")
                        if (newPersonality != currentPersonality) {
                            currentPersonality = newPersonality
                            transitionToNewPersonality()
                        }
                    }

                    println("DEBUG: Ricevuta domanda: $currentQuestion")
                    askQuestionWithPersonality(currentQuestion)
                }

                "behavior" -> {
                    if (json.getString("action") == "pause") {
                        val duration = json.optInt("duration", 1000)
                        performPersonalizedPause(duration)
                        requestNextQuestion()
                    }
                }

                "reaction" -> {
                    val message = json.getString("message")
                    val style = json.optString("style", "neutral")
                    performCustomReaction(message, style)
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
            println("DEBUG: Nessun messaggio dal server, terminando conversazione")
            endConversation()
        }

    } catch (e: Exception) {
        println("ERROR: Errore nella comunicazione con il server: ${e.message}")
        endConversation()
    }
}

private fun FlowControlRunner.transitionToNewPersonality() {
    println("DEBUG: Transizione verso personalità: $currentPersonality")

    when (currentPersonality.lowercase()) {
        "riservato" -> {
            furhat.attend(Location(0.3, -0.1, 1.0))
            furhat.say("Ehm... scusami se cambio un po' atteggiamento...", async = true)
            delay(400)
        }

        "aperto" -> {
            furhat.gesture(Gestures.BigSmile)
            furhat.attend(users.current ?: users.random)
            furhat.say("Sai cosa? Mi sento molto più a mio agio ora!", async = true)
            delay(300)
            furhat.gesture(Gestures.Nod)
        }
    }
}

private fun FlowControlRunner.performPersonalizedPause(duration: Int) {
    when (currentPersonality.lowercase()) {
        "riservato" -> {
            // Pausa riservata - comportamenti contemplativi
            furhat.gesture(Gestures.Thoughtful)
            furhat.attend(Location(0.4, -0.2, 1.0))
            Thread.sleep((duration * 1.2).toLong()) // Pausa più lunga
        }

        "aperto" -> {
            // Pausa aperta - mantiene energia
            furhat.gesture(Gestures.Smile)
            furhat.attend(users.current ?: users.random)
            Thread.sleep((duration * 0.8).toLong()) // Pausa più breve
        }

        else -> {
            Thread.sleep(duration.toLong())
        }
    }
}

private fun FlowControlRunner.performCustomReaction(message: String, style: String) {
    when (currentPersonality.lowercase()) {
        "riservato" -> {
            when (style) {
                "hesitant" -> {
                    furhat.gesture(Gestures.CloseEyes, async = true)
                    delay(300)
                    furhat.attend(Location(0.2, -0.1, 1.0))
                    furhat.say(message, async = true)
                    furhat.gesture(Gestures.Thoughtful)
                }
                "enthusiastic" -> {
                    // Anche se entusiasta, rimane riservato
                    furhat.gesture(Gestures.Surprise, async = true)
                    delay(200)
                    furhat.attend(Location(0.1, 0.0, 1.0))
                    furhat.say(message, async = true)
                    furhat.gesture(Gestures.Smile)
                }
                else -> {
                    furhat.gesture(Gestures.Thoughtful)
                    furhat.attend(Location(0.3, 0.0, 1.0))
                    furhat.say(message)
                }
            }
        }

        "aperto" -> {
            when (style) {
                "hesitant" -> {
                    // Anche se esitante, mantiene apertura
                    furhat.gesture(Gestures.Thoughtful)
                    furhat.attend(users.current ?: users.random)
                    furhat.say(message, async = true)
                    delay(300)
                    furhat.gesture(Gestures.Smile)
                }
                "enthusiastic" -> {
                    furhat.gesture(Gestures.BigSmile)
                    furhat.attend(users.current ?: users.random)
                    furhat.say(message, async = true)
                    delay(300)
                    furhat.gesture(Gestures.Nod)
                }
                else -> {
                    furhat.gesture(Gestures.Smile)
                    furhat.attend(users.current ?: users.random)
                    furhat.say(message)
                }
            }
        }

        else -> {
            // Comportamento standard basato solo sullo style
            when (style) {
                "hesitant" -> {
                    furhat.gesture(Gestures.Thoughtful)
                    furhat.say(message)
                }
                "enthusiastic" -> {
                    furhat.gesture(Gestures.BigSmile)
                    furhat.attend(users.current ?: users.random)
                    furhat.say(message)
                }
                else -> {
                    furhat.gesture(Gestures.Nod)
                    furhat.say(message)
                }
            }
        }
    }
}

private fun FlowControlRunner.endConversationWithPersonality(message: String) {
    // Imposta il flag per fermare il listening
    conversationEnded = true
    isWaitingForResponse = false

    delay(800)

    // Ferma tutti i comportamenti asincroni in corso
    furhat.stopSpeaking()

    // Posizione neutra finale
    furhat.attend(users.current ?: users.random)

    // Solo il messaggio dal server, senza sovrapposizioni
    furhat.say(message)

    // Gesto di chiusura semplice
    furhat.gesture(Gestures.Nod)

    // Attesa breve prima di passare a Idle
    delay(500)

    println("DEBUG: Conversazione terminata, chiusura Client")
    try {
        server.close()
        println("DEBUG: Connessione al server chiusa")
    } catch (e: Exception) {
        println("ERRORE nella chiusura della connessione: ${e.message}")
    }

    // Chiude completamente il client
    System.exit(0)
}

private fun FlowControlRunner.endConversation() {
    // Messaggio di default semplice
    val defaultMessage = "Grazie per la conversazione."
    endConversationWithPersonality(defaultMessage)
}

private fun FlowControlRunner.askQuestionWithPersonality(question: String) {
    isWaitingForResponse = true

    when (currentPersonality.lowercase()) {
        "riservato" -> {
            // Approccio timido e riservato
            performReservedQuestionBehavior(question)
        }

        "aperto" -> {
            // Approccio aperto e confidente
            performOpenQuestionBehavior(question)
        }

        else -> {
            // Approccio neutro
            performNeutralQuestionBehavior(question)
        }
    }

    furhat.listen()
}

private fun FlowControlRunner.performReservedQuestionBehavior(question: String) {
    // Comportamento pre-domanda riservato SENZA chiudere occhi
    furhat.gesture(Gestures.Thoughtful)
    delay(400)

    // Evita contatto visivo diretto, guarda leggermente di lato
    val avoidancePositions = arrayOf(
        Location(0.3, -0.1, 1.0),
        Location(-0.2, 0.1, 1.0),
        Location(0.4, 0.0, 1.0)
    )
    furhat.attend(avoidancePositions.random())

    // Pausa riflessiva casuale
    delay((600 + Random.nextInt(800)).toLong())

    furhat.gesture(Gestures.Thoughtful, async = true)
    delay(200)

    // Pone la domanda in modo esitante
    furhat.say(question, async = false)

    // Comportamento post-domanda - guarda altrove ma SENZA chiudere occhi
    delay(300)
    furhat.attend(Location(0.2, -0.2, 1.0))
}

private fun FlowControlRunner.performOpenQuestionBehavior(question: String) {
    // Comportamento pre-domanda aperto
    furhat.gesture(Gestures.BigSmile)
    furhat.attend(users.current ?: users.random)

    // Pausa energica ma breve
    delay(200)

    // Gesto di apertura
    furhat.gesture(Gestures.Nod, async = true)
    delay(300)

    // Pone la domanda con energia
    furhat.say(question, async = false)

    // Comportamento post-domanda incoraggiante
    delay(400)
    furhat.gesture(Gestures.Smile, async = true)

    // Mantiene contatto visivo diretto
    furhat.attend(users.current ?: users.random)

    // Piccolo gesto di incoraggiamento
    delay(200)
    furhat.gesture(Gestures.Nod, async = true)
}

private fun FlowControlRunner.performNeutralQuestionBehavior(question: String) {
    furhat.gesture(Gestures.Smile)
    furhat.attend(users.current ?: users.random)
    delay(300)
    furhat.say(question, async = false)
    delay(200)
    furhat.gesture(Gestures.Nod)
}