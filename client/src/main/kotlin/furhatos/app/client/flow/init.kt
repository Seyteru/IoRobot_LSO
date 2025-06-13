package furhatos.app.client.flow

import furhatos.app.client.flow.main.Idle
import furhatos.app.client.flow.main.Greeting
import furhatos.app.client.network.ServerConnection
import furhatos.app.client.setting.DISTANCE_TO_ENGAGE
import furhatos.app.client.setting.MAX_NUMBER_OF_USERS
import furhatos.flow.kotlin.State
import furhatos.flow.kotlin.furhat
import furhatos.flow.kotlin.state
import furhatos.flow.kotlin.users
import furhatos.util.Language

lateinit var server: ServerConnection


val Init: State = state {
    init {
        /** Set our default interaction parameters */
        users.setSimpleEngagementPolicy(DISTANCE_TO_ENGAGE, MAX_NUMBER_OF_USERS)
        furhat.setInputLanguage(Language.ITALIAN)
    }
    onEntry {
        /** start interaction */
        try {
            server = ServerConnection("127.0.0.1", 5555)
        } catch (e: Exception) {
            furhat.say("Errore di connessione con il server all’avvio.")
            goto(Idle)
            return@onEntry
        }
        when {
            furhat.isVirtual() -> goto(Greeting) // Convenient to bypass the need for user when running Virtual Furhat
            users.hasAny() -> {
                furhat.attend(users.random)
                goto(Greeting)
            }
            else -> goto(Idle)
        }
    }

}
