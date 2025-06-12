package furhatos.app.client.utils

import furhatos.nlu.Intent
import furhatos.util.Language

class NumberIntent(val number: Int = 0) : Intent() {

    override fun getExamples(lang: Language): List<String> {
        // Esempi validi: solo le cifre 1–7
        return listOf("1", "2", "3", "4", "5", "6", "7")
    }
}