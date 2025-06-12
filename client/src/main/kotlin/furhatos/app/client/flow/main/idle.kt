package furhatos.app.client.flow.main

import furhatos.flow.kotlin.State
import furhatos.flow.kotlin.furhat
import furhatos.flow.kotlin.onResponse
import furhatos.flow.kotlin.onUserEnter
import furhatos.flow.kotlin.state

val Idle: State = state {
    onEntry {
        furhat.attendNobody()
        furhat.listen()
    }

    onResponse {
        furhat.say("Scusa, mi ero distratta.")
        goto(Greeting)
    }

    onUserEnter {
        furhat.attend(it)
        goto(Greeting)
    }

}
