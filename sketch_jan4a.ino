#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <SoftwareSerial.h>
 
// ========== CONFIGURATION ==========
// WiFi Credentials
const char* ssid = "Ankam";
const char* password = "9866217694";

// Groq API Configuration
const char* apiKey = "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";
const char* apiEndpoint = "https://api.groq.com/openai/v1/chat/completions";

// Pin Definitions
#define DHTPIN D4
#define DHTTYPE DHT11  // Change to DHT22 if you have that
#define JOYSTICK_X A0
#define JOYSTICK_BTN D3  // ⚠️ THIS executes menu options (calls AI)
#define MANUAL_BTN D5    // ⚠️ THIS only shows quick sensor reading
#define BT_RX D6         // Connect to HC-05 TX
#define BT_TX D7         // Connect to HC-05 RX

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

// Bluetooth Serial
SoftwareSerial BTSerial(BT_RX, BT_TX); // RX, TX

// Global Variables
float temperature = 0;
float humidity = 0;
int currentMenu = 0;
unsigned long lastSensorRead = 0;
unsigned long lastJoystickMove = 0;
const unsigned long sensorInterval = 5000; // Read sensor every 5 seconds

// Mode flags
bool gameMode = false;
int currentGame = 0;

// Game state variables
int secretNumber = 0;
int guessCount = 0;
String riddleAnswer = "";
int triviaScore = 0;

// Menu Items
const char* menuItems[] = {
  "1. Check Environment",
  "2. Get Comfort Tips",
  "3. Health Advisory",
  "4. Energy Saving Tips",
  "5. Air Quality Advice"
};
const int menuCount = 5;

// Game Menu
const char* gameItems[] = {
  "1. Number Guessing",
  "2. Riddle Challenge",
  "3. Trivia Quiz",
  "4. Story Creator",
  "5. Exit Game Mode"
};
const int gameCount = 5;

// Button states
bool lastBtnState = HIGH;
bool lastManualBtnState = HIGH;

// ========== BLUETOOTH PRINT FUNCTIONS ==========
void BTPrint(String text) {
  BTSerial.print(text);
}

void BTPrintln(String text = "") {
  BTSerial.println(text);
}

void dualPrint(String text) {
  Serial.print(text);
  BTSerial.print(text);
}

void dualPrintln(String text = "") {
  Serial.println(text);
  BTSerial.println(text);
}

// ========== MEMORY MONITORING ==========
void checkMemory() {
  uint32_t free = ESP.getFreeHeap();
  Serial.println("Free Heap: " + String(free) + " bytes");
  if (free < 10000) {
    Serial.println("⚠️ WARNING: Low memory!");
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);  // Debug monitor
  BTSerial.begin(9600);  // Bluetooth HC-05/HC-06 default baud rate
  delay(1000);
  
  // Initialize DHT
  dht.begin();
  
  // Initialize Pins
  pinMode(JOYSTICK_BTN, INPUT_PULLUP);
  pinMode(MANUAL_BTN, INPUT_PULLUP);
  
  // Debug info to Serial only
  Serial.println("\n\n╔════════════════════════════════╗");
  Serial.println("║  Smart Home Monitor v3.0       ║");
  Serial.println("║  (With AI Game Mode!)          ║");
  Serial.println("╚════════════════════════════════╝");
  Serial.println("\nConnecting to WiFi...");
  
  // Welcome message to Bluetooth
  BTPrintln("\n\n╔════════════════════════════════╗");
  BTPrintln("║  Smart Home Monitor v3.0       ║");
  BTPrintln("║  Connected via Bluetooth!      ║");
  BTPrintln("║  🎮 NOW WITH GAMES! 🎮         ║");
  BTPrintln("╚════════════════════════════════╝");
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
    
    BTPrintln("\n✓ System Ready!");
    BTPrintln("WiFi: Connected");
  } else {
    Serial.println("\n✗ WiFi Failed - Check credentials");
    BTPrintln("\n⚠ WiFi connection failed");
    BTPrintln("Limited functionality available");
  }
  
  // Instructions to Bluetooth
  BTPrintln("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  BTPrintln("MOBILE CONTROLS:");
  BTPrintln("• Type 1-5: Select menu option");
  BTPrintln("• Type 'game': Enter game mode 🎮");
  BTPrintln("• Type 'help': Show commands");
  BTPrintln("• Type 'status': Show sensors");
  BTPrintln("• Type 'menu': Show menu");
  BTPrintln("• Or ask any question!");
  BTPrintln("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  
  checkMemory();
  
  // Initial sensor reading
  readSensors();
  displayMenu();
}

// ========== MAIN LOOP ==========
void loop() {
  // Read sensors periodically
  if (millis() - lastSensorRead >= sensorInterval) {
    readSensors();
    lastSensorRead = millis();
  }
  
  // Handle Joystick Navigation
  handleJoystick();
  
  // Handle Manual Button (optional quick sensor read)
  handleManualButton();
  
  // Handle Bluetooth Commands
  if (BTSerial.available()) {
    handleBluetoothCommand();
  }
  
  // Handle Serial Commands (for debugging)
  if (Serial.available()) {
    handleSerialCommand();
  }
  
  delay(50);
}

// ========== SENSOR READING ==========
void readSensors() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("⚠ WARNING: Failed to read DHT sensor!");
    humidity = 0;
    temperature = 0;
  }
}

// ========== MENU DISPLAY ==========
void displayMenu() {
  if (gameMode) {
    displayGameMenu();
    return;
  }
  
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║          MAIN MENU             ║");
  BTPrintln("╠════════════════════════════════╣");
  
  for (int i = 0; i < menuCount; i++) {
    String line = "║ ";
    if (i == currentMenu) {
      line += "► ";
    } else {
      line += "  ";
    }
    line += menuItems[i];
    
    // Padding for alignment
    int padding = 28 - strlen(menuItems[i]);
    for (int j = 0; j < padding; j++) line += " ";
    line += "║";
    
    BTPrintln(line);
  }
  
  BTPrintln("╠════════════════════════════════╣");
  BTPrintln("║ Type 'game' for Game Mode! 🎮 ║");
  BTPrintln("╚════════════════════════════════╝");
  BTPrintln("Current: Temp=" + String(temperature, 1) + "°C, Humidity=" + String(humidity, 1) + "%\n");
  
  Serial.println("Main menu displayed. Current selection: " + String(currentMenu + 1));
}

// ========== GAME MENU DISPLAY ==========
void displayGameMenu() {
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║       🎮 GAME MODE 🎮          ║");
  BTPrintln("╠════════════════════════════════╣");
  
  for (int i = 0; i < gameCount; i++) {
    String line = "║ ";
    if (i == currentGame) {
      line += "► ";
    } else {
      line += "  ";
    }
    line += gameItems[i];
    
    // Padding for alignment
    int padding = 28 - strlen(gameItems[i]);
    for (int j = 0; j < padding; j++) line += " ";
    line += "║";
    
    BTPrintln(line);
  }
  
  BTPrintln("╚════════════════════════════════╝\n");
  
  Serial.println("Game menu displayed. Current selection: " + String(currentGame + 1));
}

// ========== JOYSTICK HANDLING ==========
void handleJoystick() {
  int xValue = analogRead(JOYSTICK_X);
  bool btnState = digitalRead(JOYSTICK_BTN);
  
  int maxMenu = gameMode ? gameCount : menuCount;
  int* menuPtr = gameMode ? &currentGame : &currentMenu;
  
  // Menu navigation with debounce
  if (millis() - lastJoystickMove > 300) {
    if (xValue < 300) {  // Left - Move up
      *menuPtr = (*menuPtr - 1 + maxMenu) % maxMenu;
      displayMenu();
      Serial.println("Joystick: Moved UP to option " + String(*menuPtr + 1));
      lastJoystickMove = millis();
    } else if (xValue > 700) {  // Right - Move down
      *menuPtr = (*menuPtr + 1) % maxMenu;
      displayMenu();
      Serial.println("Joystick: Moved DOWN to option " + String(*menuPtr + 1));
      lastJoystickMove = millis();
    }
  }
  
  // Button press detection (with debounce)
  if (btnState == LOW && lastBtnState == HIGH) {
    delay(50);  // Debounce
    if (digitalRead(JOYSTICK_BTN) == LOW) {
      if (gameMode) {
        BTPrintln("\n▶ Starting Game " + String(currentGame + 1) + "...");
        Serial.println("Starting game " + String(currentGame + 1));
        executeGameOption(currentGame + 1);
      } else {
        BTPrintln("\n▶ Executing option " + String(currentMenu + 1) + "...");
        Serial.println("Executing option " + String(currentMenu + 1));
        executeMenuOption(currentMenu + 1);
      }
      delay(300);
    }
  }
  lastBtnState = btnState;
}

// ========== MANUAL BUTTON HANDLING ==========
void handleManualButton() {
  bool manualBtnState = digitalRead(MANUAL_BTN);
  
  if (manualBtnState == LOW && lastManualBtnState == HIGH) {
    delay(50);  // Debounce
    if (digitalRead(MANUAL_BTN) == LOW) {
      readSensors();
      BTPrintln("\n┌─────────────────────────┐");
      BTPrintln("│ ⚡ QUICK SENSOR READ    │");
      BTPrintln("├─────────────────────────┤");
      BTPrintln("│ Temp: " + String(temperature, 1) + "°C");
      BTPrintln("│ Humidity: " + String(humidity, 1) + "%");
      BTPrintln("└─────────────────────────┘\n");
      
      Serial.println("Manual button pressed");
      delay(300);
    }
  }
  lastManualBtnState = manualBtnState;
}

// ========== BLUETOOTH COMMAND HANDLING ==========
void handleBluetoothCommand() {
  String command = BTSerial.readStringUntil('\n');
  command.trim();
  
  if (command.length() == 0) return;
  
  Serial.println("BT Command: " + command);
  BTPrintln("\n> " + command);
  
  String cmdLower = command;
  cmdLower.toLowerCase();
  
  // Check for game mode toggle
  if (cmdLower == "game" || cmdLower == "g") {
    toggleGameMode();
    return;
  }
  
  // If in game mode, handle game input
  if (gameMode) {
    handleGameInput(command);
    return;
  }
  
  // Regular commands
  if (cmdLower == "help" || cmdLower == "h") {
    displayHelp();
  } else if (cmdLower == "status" || cmdLower == "s") {
    showStatus();
  } else if (cmdLower == "menu" || cmdLower == "m") {
    displayMenu();
  } else if (command >= "1" && command <= "5") {
    int option = command.toInt();
    executeMenuOption(option);
  } else {
    processCustomQuestion(command);
  }
}

// ========== SERIAL COMMAND HANDLING ==========
void handleSerialCommand() {
  String command = Serial.readStringUntil('\n');
  command.trim();
  
  if (command.length() == 0) return;
  
  Serial.println("\nSerial Command: " + command);
  
  String cmdLower = command;
  cmdLower.toLowerCase();
  
  if (cmdLower == "game" || cmdLower == "g") {
    toggleGameMode();
    return;
  }
  
  if (gameMode) {
    handleGameInput(command);
    return;
  }
  
  if (cmdLower == "help" || cmdLower == "h") {
    displayHelp();
  } else if (cmdLower == "status" || cmdLower == "s") {
    showStatus();
  } else if (cmdLower == "menu" || cmdLower == "m") {
    displayMenu();
  } else if (command >= "1" && command <= "5") {
    int option = command.toInt();
    executeMenuOption(option);
  } else {
    processCustomQuestion(command);
  }
}

// ========== GAME MODE TOGGLE ==========
void toggleGameMode() {
  gameMode = !gameMode;
  currentGame = 0;
  
  if (gameMode) {
    BTPrintln("\n🎮 ════════════════════════════ 🎮");
    BTPrintln("║   ENTERING GAME MODE!        ║");
    BTPrintln("🎮 ════════════════════════════ 🎮");
    BTPrintln("\nGet ready for some AI-powered fun!");
    BTPrintln("Type 'game' again to exit.\n");
    Serial.println(">>> GAME MODE ACTIVATED <<<");
  } else {
    BTPrintln("\n👋 Exiting Game Mode...");
    BTPrintln("Returning to main menu.\n");
    Serial.println(">>> GAME MODE DEACTIVATED <<<");
    currentMenu = 0;
  }
  
  displayMenu();
}

// ========== HELP DISPLAY ==========
void displayHelp() {
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║        AVAILABLE COMMANDS      ║");
  BTPrintln("╠════════════════════════════════╣");
  if (gameMode) {
    BTPrintln("║ 1-5     : Select game          ║");
    BTPrintln("║ game/g  : Exit game mode       ║");
  } else {
    BTPrintln("║ 1-5     : Select menu option   ║");
    BTPrintln("║ game/g  : Enter game mode 🎮   ║");
  }
  BTPrintln("║ help/h  : Show this help       ║");
  BTPrintln("║ status/s: Show sensor data     ║");
  BTPrintln("║ menu/m  : Show menu again      ║");
  if (!gameMode) {
    BTPrintln("║ [text]  : Ask custom question  ║");
  }
  BTPrintln("╚════════════════════════════════╝\n");
}

// ========== STATUS DISPLAY ==========
void showStatus() {
  readSensors();
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║       SYSTEM STATUS            ║");
  BTPrintln("╠════════════════════════════════╣");
  BTPrintln("║ WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected ✓" : "Disconnected ✗") + "          ║");
  
  String ipStr = WiFi.localIP().toString();
  if (ipStr.length() < 15) {
    while (ipStr.length() < 15) ipStr += " ";
  }
  BTPrintln("║ IP: " + ipStr + "     ║");
  BTPrintln("║                                ║");
  
  String tempStr = String(temperature, 1);
  BTPrintln("║ Temperature: " + tempStr + "°C" + String(tempStr.length() < 4 ? "          " : "         ") + "║");
  
  String humStr = String(humidity, 1);
  BTPrintln("║ Humidity: " + humStr + "%" + String(humStr.length() < 4 ? "             " : "            ") + "║");
  BTPrintln("║                                ║");
  
  String comfort = "Unknown";
  if (temperature >= 20 && temperature <= 26 && humidity >= 40 && humidity <= 60) {
    comfort = "Optimal ✓";
  } else if (temperature > 28 || humidity > 70) {
    comfort = "Too Hot/Humid";
  } else if (temperature < 18 || humidity < 30) {
    comfort = "Too Cold/Dry";
  }
  
  BTPrintln("║ Comfort: " + comfort + String(comfort.length() < 15 ? "            " : "") + "║");
  
  if (gameMode) {
    BTPrintln("║ Mode: 🎮 GAME MODE            ║");
  }
  
  BTPrintln("╚════════════════════════════════╝\n");
  
  checkMemory();
}

// ========== MENU EXECUTION ==========
void executeMenuOption(int option) {
  String prompt = "";
  
  readSensors();
  
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║  EXECUTING OPTION " + String(option) + "            ║");
  BTPrintln("╚════════════════════════════════╝\n");
  Serial.println(">>> Executing Option " + String(option) + " <<<");
  
  switch(option) {
    case 1:
      BTPrintln("📊 Current Environment Status:");
      showStatus();
      return;
      
    case 2:
      BTPrintln("🌡️ Getting Comfort Tips...\n");
      prompt = "Room is " + String(temperature, 1) + "°C and " + 
               String(humidity, 1) + "% humidity. Give 2 quick comfort tips in under 70 words.";
      break;
      
    case 3:
      BTPrintln("🏥 Getting Health Advisory...\n");
      prompt = "Temperature " + String(temperature, 1) + "°C, humidity " + 
               String(humidity, 1) + "%. Any health concerns? Answer in 50 words.";
      break;
      
    case 4:
      BTPrintln("💡 Getting Energy Saving Tips...\n");
      prompt = "With " + String(temperature, 1) + "°C and " + 
               String(humidity, 1) + "% humidity indoors, suggest 2 energy-saving tips. Brief, 50 words max.";
      break;
      
    case 5:
      BTPrintln("🌬️ Getting Air Quality Advice...\n");
      prompt = "Room temp " + String(temperature, 1) + "°C, humidity " + 
               String(humidity, 1) + "%. Should I open windows? Brief advice, 40 words.";
      break;
      
    default:
      BTPrintln("✗ Invalid option! Please select 1-5");
      return;
  }
  
  BTPrintln("⏳ Connecting to AI...");
  checkMemory();
  
  String response = callLLM(prompt);
  
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║        AI RESPONSE             ║");
  BTPrintln("╠════════════════════════════════╣");
  printWrapped(response, 30);
  BTPrintln("╚════════════════════════════════╝\n");
  
  delay(500);
  displayMenu();
}

// ========== CUSTOM QUESTION ==========
void processCustomQuestion(String question) {
  BTPrintln("\n💬 Processing your question...");
  Serial.println("Custom question: " + question);
  
  String prompt = "Room: " + String(temperature, 1) + "°C, " + 
                  String(humidity, 1) + "%. Q: \"" + question + 
                  "\". Answer in 60 words.";
  
  checkMemory();
  String response = callLLM(prompt);
  
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║        AI RESPONSE             ║");
  BTPrintln("╠════════════════════════════════╣");
  printWrapped(response, 30);
  BTPrintln("╚════════════════════════════════╝\n");
  
  delay(500);
  displayMenu();
}

// ========== GAME EXECUTION ==========
void executeGameOption(int gameNum) {
  checkMemory();
  
  switch(gameNum) {
    case 1:
      startNumberGuessingGame();
      break;
    case 2:
      startRiddleGame();
      break;
    case 3:
      startTriviaGame();
      break;
    case 4:
      startStoryGame();
      break;
    case 5:
      toggleGameMode();
      break;
    default:
      BTPrintln("✗ Invalid game option!");
      break;
  }
}

// ========== GAME INPUT HANDLER ==========
void handleGameInput(String input) {
  // Check if they want to quit
  if (input.equalsIgnoreCase("quit") || input.equalsIgnoreCase("exit") || 
      input.equalsIgnoreCase("menu") || input.equalsIgnoreCase("back")) {
    BTPrintln("\n👋 Returning to game menu...\n");
    displayGameMenu();
    return;
  }
  
  // Check if it's a game selection
  if (input.length() == 1 && input[0] >= '1' && input[0] <= '5') {
    int gameNum = input.toInt();
    executeGameOption(gameNum);
    return;
  }
  
  // Otherwise, treat as game response
  BTPrintln("\n💭 Thinking...");
  
  String prompt = "User says: \"" + input + "\". Respond in 50 words.";
  
  checkMemory();
  String response = callLLM(prompt);
  
  BTPrintln("\n" + response + "\n");
  BTPrintln("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  BTPrintln("(Type your response or 'menu')");
}

// ========== GAME 1: NUMBER GUESSING ==========
void startNumberGuessingGame() {
  randomSeed(analogRead(A0) + millis());
  secretNumber = random(1, 101);
  guessCount = 0;
  
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║    🔢 NUMBER GUESSING GAME     ║");
  BTPrintln("╠════════════════════════════════╣");
  BTPrintln("║ I'm thinking of a number      ║");
  BTPrintln("║ between 1 and 100.            ║");
  BTPrintln("║                                ║");
  BTPrintln("║ Type your guess!              ║");
  BTPrintln("║ (or 'hint' for AI help)       ║");
  BTPrintln("╚════════════════════════════════╝\n");
  
  Serial.println("Secret number: " + String(secretNumber));
  
  // Wait for user input
  while(true) {
    if (BTSerial.available()) {
      String guess = BTSerial.readStringUntil('\n');
      guess.trim();
      
      if (guess.equalsIgnoreCase("menu") || guess.equalsIgnoreCase("quit")) {
        displayGameMenu();
        return;
      }
      
      if (guess.equalsIgnoreCase("hint")) {
        BTPrintln("\n💡 Getting AI hint...");
        String prompt = "The secret number is " + String(secretNumber) + 
                       ". Give a cryptic hint in 25 words.";
        checkMemory();
        String hint = callLLM(prompt);
        BTPrintln("\n🤖 AI: " + hint + "\n");
        continue;
      }
      
      int guessNum = guess.toInt();
      if (guessNum < 1 || guessNum > 100) {
        BTPrintln("❌ Please enter a number between 1 and 100!\n");
        continue;
      }
      
      guessCount++;
      
      if (guessNum == secretNumber) {
        BTPrintln("\n🎉 ═══════════════════════ 🎉");
        BTPrintln("║  CORRECT! YOU WIN!      ║");
        BTPrintln("🎉 ═══════════════════════ 🎉");
        BTPrintln("\nYou guessed it in " + String(guessCount) + " tries!");
        BTPrintln("The number was " + String(secretNumber) + "\n");
        delay(2000);
        displayGameMenu();
        return;
      } else if (guessNum < secretNumber) {
        BTPrintln("📈 Too low! Try again.\n");
      } else {
        BTPrintln("📉 Too high! Try again.\n");
      }
      
      BTPrintln("Guesses: " + String(guessCount));
    }
    delay(100);
  }
}

// ========== GAME 2: RIDDLE CHALLENGE ==========
void startRiddleGame() {
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║     🤔 RIDDLE CHALLENGE        ║");
  BTPrintln("╠════════════════════════════════╣");
  BTPrintln("║ Generating a riddle...         ║");
  BTPrintln("╚════════════════════════════════╝\n");
  
  String prompt = "Create a simple riddle with answer. Format: 'Riddle: [riddle]. Answer: [answer]'. Keep it under 40 words total.";
  
  checkMemory();
  String response = callLLM(prompt);
  
  // Extract answer (after "Answer:")
  int ansIdx = response.indexOf("Answer:");
  if (ansIdx == -1) ansIdx = response.indexOf("answer:");
  
  if (ansIdx > 0) {
    String riddle = response.substring(0, ansIdx);
    riddleAnswer = response.substring(ansIdx + 7);
    riddleAnswer.trim();
    riddleAnswer.toLowerCase();
    
    BTPrintln(riddle);
    BTPrintln("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    BTPrintln("Type your answer:");
    BTPrintln("(or 'reveal' to see answer)\n");
    
    Serial.println("Riddle answer: " + riddleAnswer);
    
    // Wait for answer
    while(true) {
      if (BTSerial.available()) {
        String answer = BTSerial.readStringUntil('\n');
        answer.trim();
        answer.toLowerCase();
        
        if (answer.equalsIgnoreCase("menu") || answer.equalsIgnoreCase("quit")) {
          displayGameMenu();
          return;
        }
        
        if (answer.equalsIgnoreCase("reveal")) {
          BTPrintln("\n💡 The answer was: " + riddleAnswer + "\n");
          delay(2000);
          displayGameMenu();
          return;
        }
        
        if (riddleAnswer.indexOf(answer) >= 0 || answer.indexOf(riddleAnswer.substring(0, min(5, (int)riddleAnswer.length()))) >= 0) {
          BTPrintln("\n🎉 Correct! Well done! 🎉\n");
          delay(2000);
          displayGameMenu();
          return;
        } else {
          BTPrintln("\n❌ Not quite. Try again!\n");
        }
      }
      delay(100);
    }
  } else {
    BTPrintln("\n⚠️ Couldn't generate riddle properly.\n");
    delay(1000);
    displayGameMenu();
  }
}

// ========== GAME 3: TRIVIA QUIZ ==========
void startTriviaGame() {
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║      🧠 TRIVIA QUIZ            ║");
  BTPrintln("╠════════════════════════════════╣");
  BTPrintln("║ Generating question...         ║");
  BTPrintln("╚════════════════════════════════╝\n");
  
  String prompt = "Ask a simple trivia question. Format: 'Q: [question]? A: [answer]'. Easy difficulty, 30 words max.";
  
  checkMemory();
  String response = callLLM(prompt);
  
  // Extract answer
  int ansIdx = response.indexOf("A:");
  if (ansIdx == -1) ansIdx = response.indexOf("Answer:");
  
  if (ansIdx > 0) {
    String question = response.substring(0, ansIdx);
    String answer = response.substring(ansIdx + 2);
    if (response.indexOf("Answer:") >= 0) answer = response.substring(ansIdx + 7);
    answer.trim();
    
    BTPrintln(question);
    BTPrintln("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    BTPrintln("Type your answer:\n");
    
    Serial.println("Trivia answer: " + answer);
    
    // Wait for answer
    if (BTSerial.available()) BTSerial.readString(); // Clear buffer
    
    while(true) {
      if (BTSerial.available()) {
        String userAnswer = BTSerial.readStringUntil('\n');
        userAnswer.trim();
        
        if (userAnswer.equalsIgnoreCase("menu") || userAnswer.equalsIgnoreCase("quit")) {
          displayGameMenu();
          return;
        }
        
        // Ask AI to verify answer
        BTPrintln("\n🤔 Checking your answer...");
        String checkPrompt = "The correct answer is '" + answer + "'. User answered '" + 
                            userAnswer + "'. Is it correct? Reply 'YES' or 'NO' only.";
        
        checkMemory();
        String verdict = callLLM(checkPrompt);
        verdict.toUpperCase();
        
        if (verdict.indexOf("YES") >= 0) {
          BTPrintln("\n✅ Correct! Great job! ✅");
          BTPrintln("Answer: " + answer + "\n");
        } else {
          BTPrintln("\n❌ Not quite!");
          BTPrintln("The answer was: " + answer + "\n");
        }
        
        delay(2000);
        displayGameMenu();
        return;
      }
      delay(100);
    }
  } else {
    BTPrintln("\n⚠️ Couldn't generate trivia properly.\n");
    delay(1000);
    displayGameMenu();
  }
}

// ========== GAME 4: STORY CREATOR ==========
void startStoryGame() {
  BTPrintln("\n╔════════════════════════════════╗");
  BTPrintln("║     📖 STORY CREATOR           ║");
  BTPrintln("╠════════════════════════════════╣");
  BTPrintln("║ Let's create a story together! ║");
  BTPrintln("║                                ║");
  BTPrintln("║ I'll start, then you continue. ║");
  BTPrintln("╚════════════════════════════════╝\n");
  
  String prompt = "Start a short story (2 sentences max, 30 words) with an interesting opening.";
  
  checkMemory();
  String storyStart = callLLM(prompt);
  
  BTPrintln("🤖 AI: " + storyStart);
  BTPrintln("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  BTPrintln("Your turn! What happens next?");
  BTPrintln("(Type 'end' to finish story)\n");
  
  // Wait for continuation
  while(true) {
    if (BTSerial.available()) {
      String continuation = BTSerial.readStringUntil('\n');
      continuation.trim();
      
      if (continuation.equalsIgnoreCase("menu") || continuation.equalsIgnoreCase("quit")) {
        displayGameMenu();
        return;
      }
      
      if (continuation.equalsIgnoreCase("end")) {
        BTPrintln("\n📖 Story finished! Great job!\n");
        delay(2000);
        displayGameMenu();
        return;
      }
      
      BTPrintln("\n💭 AI is thinking...");
      
      String nextPrompt = "Continue this story in 2 sentences (30 words): " + continuation;
      
      checkMemory();
      String aiResponse = callLLM(nextPrompt);
      
      BTPrintln("\n🤖 AI: " + aiResponse);
      BTPrintln("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      BTPrintln("Continue the story:\n");
    }
    delay(100);
  }
}

// ========== TEXT WRAPPING ==========
void printWrapped(String text, int width) {
  int start = 0;
  while (start < text.length()) {
    int end = start + width;
    if (end >= text.length()) {
      String line = "║ ";
      line += text.substring(start);
      for (int i = 0; i < width - (text.length() - start); i++) line += " ";
      line += "║";
      BTPrintln(line);
      break;
    }
    
    int lastSpace = text.lastIndexOf(' ', end);
    if (lastSpace <= start) lastSpace = end;
    
    String line = "║ ";
    String segment = text.substring(start, lastSpace);
    line += segment;
    for (int i = 0; i < width - segment.length(); i++) line += " ";
    line += "║";
    BTPrintln(line);
    
    start = lastSpace + 1;
  }
}

// ========== LLM API CALL ==========
String callLLM(String prompt) {
  Serial.println("\n=== LLM API CALL ===");
  Serial.println("Free Heap Before: " + String(ESP.getFreeHeap()));
  
  if (WiFi.status() != WL_CONNECTED) {
    String error = "ERROR: WiFi not connected";
    Serial.println(error);
    return error;
  }
  
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient http;
  http.begin(client, apiEndpoint);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(apiKey));
  http.setTimeout(20000);
  
  // Use StaticJsonDocument to save memory
  StaticJsonDocument<512> doc;
  doc["model"] = "llama-3.3-70b-versatile";
  doc["max_tokens"] = 100;  // Reduced for memory
  doc["temperature"] = 0.8;
  
  JsonArray messages = doc.createNestedArray("messages");
  JsonObject message = messages.createNestedObject();
  message["role"] = "user";
  message["content"] = prompt;
  
  String requestBody;
  serializeJson(doc, requestBody);
  
  Serial.println("Sending request...");
  int httpCode = http.POST(requestBody);
  Serial.println("HTTP Code: " + String(httpCode));
  
  String response = "No response";
  
  if (httpCode == 200) {
    String payload = http.getString();
    
    // Use DynamicJsonDocument for response (needs variable size)
    DynamicJsonDocument responseDoc(3072);  // Reduced size
    DeserializationError error = deserializeJson(responseDoc, payload);
    
    if (!error) {
      response = responseDoc["choices"][0]["message"]["content"].as<String>();
      response.trim();
      Serial.println("✓ Success");
    } else {
      response = "Parse error: " + String(error.c_str());
      Serial.println("✗ " + response);
    }
    
    // Clear the document to free memory
    responseDoc.clear();
    payload = "";  // Clear payload string
    
  } else {
    response = "HTTP Error: " + String(httpCode);
    Serial.println("✗ " + response);
  }
  
  http.end();
  client.stop();
  
  Serial.println("Free Heap After: " + String(ESP.getFreeHeap()));
  Serial.println("=== API CALL DONE ===\n");
  
  return response;
}