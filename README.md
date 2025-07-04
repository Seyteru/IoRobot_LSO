# IoRobot_LSO

A conversational robot system for personality assessment and adaptive follow-up conversations using Furhat robot and GPT integration.

## Project Overview

This project implements an intelligent conversational robot that conducts personality assessments and engages in adaptive follow-up conversations. The system consists of a C server backend with GPT integration and Kotlin clients for the Furhat robot platform.

## Architecture

### Components

- **Server (C)**: Multi-threaded TCP server handling client connections, personality assessment logic, and GPT integration
- **Client (Kotlin)**: Furhat robot skill implementing conversational states and personality-adaptive behaviors

### Key Features

- **Personality Assessment**: 10-question Likert scale assessment to determine extroversion levels
- **GPT Integration**: Dynamic question generation and conversational responses using OpenAI API
- **Adaptive Behavior**: Robot behavior adapts based on assessed personality (reserved, neutral, open)
- **Multi-threaded Architecture**: Concurrent client handling with thread-safe logging
- **Fallback System**: Static questions available when GPT is unavailable

## Project Structure

```
├── server/                 # C Backend Server
│   ├── main.c             # Main server entry point
│   ├── server.c           # Server initialization and management
│   ├── connection.c       # Client connection handling and assessment logic
│   ├── gpt_client.c       # OpenAI GPT integration
│   ├── logger.c           # Thread-safe logging system
│   ├── *.h                # Header files
│   ├── makefile           # Build configuration
│   ├── Dockerfile         # Container deployment
│   ├── gpt_prompt.txt     # GPT system prompt template
│   └── .env.example       # Environment configuration template
├── client/                # Furhat Robot Client
│   ├── src/main/kotlin/   # Kotlin source code
│   │   └── furhatos/app/client/
│   │       ├── flow/      # Conversation flow states
│   │       └── network/   # Server communication
│   ├── assets/            # Robot assets
│   ├── build.gradle       # Build configuration
└── └── skill.properties   # Skill metadata
```

## Setup and Installation

### Prerequisites

- **Server**: GCC compiler, libcurl, libjson-c
- **Client**: Java 8+, Gradle, Furhat SDK
- **GPT Integration**: OpenAI API key

### Server Setup

1. Install dependencies (Ubuntu/Debian):
```bash
cd server
make install-deps
```

2. Configure environment:
```bash
cp .env.example .env
# Edit .env and add your OpenAI API key
```

3. Build and run:
```bash
make
make run
```

### Client Setup

#### Furhat Robot Client

1. Install Furhat SDK and configure robot
2. Build and deploy:
```bash
cd client
./gradlew shadowJar
# Deploy the generated .skill file to Furhat
```

#### NoGUI Test Client

```bash
cd client_noGUI
./gradlew run
```

## Usage

### Personality Assessment Flow

1. **Greeting Phase**: Robot initiates conversation and asks for consent
2. **Assessment Phase**: 10 personality questions using Likert scale (1-7)
3. **Analysis**: Server calculates extroversion score and determines personality style
4. **Follow-up Phase**: Adaptive conversation based on assessed personality
5. **Closure**: Personality-appropriate conversation ending

### Personality Types

- **Reserved (riservato)**: Shy behavior, limited eye contact, soft speech
- **Neutral (neutro)**: Balanced interaction style
- **Open (aperto)**: Enthusiastic gestures, direct eye contact, confident speech

### GPT Integration

The system uses OpenAI's GPT models to:
- Generate dynamic personality assessment questions
- Create contextual follow-up questions
- Provide empathetic responses based on user personality
- Adapt conversation style in real-time

## Configuration

### Server Configuration (.env)
```bash
OPENAI_API_KEY=your_api_key_here
OPENAI_API_URL=https://api.openai.com/v1/chat/completions
OPENAI_MODEL=gpt-3.5-turbo
OPENAI_MAX_TOKENS=150
OPENAI_TEMPERATURE=0.7
```

### Client Configuration (skill.properties)
```properties
name = Client
language = it-IT
requiresCamera = false
requiresSpeaker = true
requiresMicrophone = true
```

## API Documentation

### Server Protocol

The server communicates with clients using JSON messages over TCP:

#### Message Types

**Assessment Questions**:
```json
{
  "type": "ask",
  "question": "Mi sento a mio agio in mezzo alla gente.",
  "questionNum": 1,
  "total": 10
}
```

**Personality Results**:
```json
{
  "type": "result",
  "personality": {
    "extroversion": 4.2,
    "style": "neutro",
    "instructions": ["neutralTone", "moderateGaze", "balanced"]
  }
}
```

**State Changes**:
```json
{
  "type": "state_change",
  "new_state": "follow_up",
  "personality": "aperto"
}
```

## Development

### Building from Source

**Server**:
```bash
cd server
make clean
make
```

**Client**:
```bash
cd client
./gradlew build
./gradlew shadowJar
```

### Testing

Use the NoGUI client for testing server functionality:
```bash
cd client_noGUI
./gradlew run
```

The server includes a comprehensive test client in `server/test.py` for stress testing and validation.

### Docker Deployment

```bash
cd server
docker build -t iorobot-server .
docker run -p 5555:5555 iorobot-server
```

## Logging and Monitoring

The server provides detailed logging with multiple levels:
- **DEBUG**: Detailed protocol messages
- **INFO**: General operation status  
- **WARNING**: Non-critical issues
- **ERROR**: Critical failures

Logs include client identification, thread information, and message tracing for debugging.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Implement changes with appropriate tests
4. Submit a pull request

## License

This project is developed for the Operating Systems Laboratory course. See LICENSE file for details.

## Contributors

- **Simone Cioffi** - Main contributor
- **Bruno De Vivo** - Main contributor

## Acknowledgments

- Furhat Robotics for the conversational robot platform
- OpenAI for GPT integration capabilities
- Course instructors and teaching assistants
