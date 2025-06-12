package furhatos.app.client

import furhatos.app.client.flow.Init
import furhatos.flow.kotlin.Flow
import furhatos.skills.Skill

class ClientSkill : Skill() {
    override fun start() {
        Flow().run(Init)
    }
}

fun main(args: Array<String>) {
    Skill.main(args)
}
