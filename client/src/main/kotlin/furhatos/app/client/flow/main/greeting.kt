package furhatos.app.client.flow.main

import furhatos.app.client.flow.Parent
import furhatos.flow.kotlin.*
import furhatos.nlu.common.No
import furhatos.nlu.common.Yes

val Greeting: State = state(Parent) {
    onEntry {
        furhat.ask("Ciao, vorrei farti qualche domanda per conoscerti meglio. Posso?")
    }

    onReentry {
        furhat.listen()
        furhat.ask("Scusa, non ti ho sentito bene.")
    }

    onResponse<Yes> {
        furhat.say("Iniziamo!")
        goto(PersonalityAssessment)
    }

    onResponse<No> {
        furhat.say("Ok.")

    }

}

