plugins {
    kotlin("jvm") version "2.1.20"
    application
}

group = "org.cioffiDeVivo"
version = "1.0-SNAPSHOT"

repositories {
    mavenCentral()
}

dependencies {
    implementation("org.json:json:20231013")
    implementation("com.squareup.okhttp3:okhttp:4.12.0")

    testImplementation(kotlin("test"))
}

tasks.test {
    useJUnitPlatform()
}
kotlin {
    jvmToolchain(17)
}

application {
    mainClass.set("org.cioffiDeVivo.MainKt")
}

tasks.named<JavaExec>("run") {
    standardInput = System.`in`
}