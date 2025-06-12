package furhatos.app.client.flow.main

import furhatos.app.client.flow.Parent
import furhatos.flow.kotlin.*

val FollowUpConversation: State = state(Parent) {
    onEntry {
        furhat.say("Adesso se ti và conversiamo!")
        goto(Idle)
    }
}