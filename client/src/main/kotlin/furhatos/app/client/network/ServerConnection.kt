package furhatos.app.client.network

import org.json.JSONObject
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.Socket

class ServerConnection(ip: String, port: Int) {
    private val socket: Socket = Socket(ip, port)
    private val input = BufferedReader(InputStreamReader(socket.getInputStream()))
    private val output = BufferedWriter(OutputStreamWriter(socket.getOutputStream()))

    fun readJson(): JSONObject? {
        val line = input.readLine() ?: return null
        return JSONObject(line)
    }

    fun sendLine(message: String) {
        output.write("$message\n")
        output.flush()
    }

    fun close() {
        socket.close()
    }
}