#include <IRremote.h>
#include <U8x8lib.h>

#define IR_RECEIVE_PIN 2

#define MOTOR_IN1 A0
#define MOTOR_IN2 A1
#define MOTOR_IN3 A2
#define MOTOR_IN4 A3

#define LED1 8
#define LED2 9

#define SENSOR1 3
#define SENSOR2 4
#define SENSOR3 5
#define SENSOR4 6
#define SENSOR5 7

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);  

uint32_t lastReceivedCode = 0;
unsigned long lastProcessedTime = 0;  

int currentMenu = 0;  // 0 = Menu Utama, 1 = Line Follow, 2 = Control IR
int selectedOption = 0;

unsigned long ledTimer = 0;
bool ledState = false;
bool lineFollowerActive = false;

void setup() {
  Serial.begin(9600);
  Serial.println("IR Receiver is ready");

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  showHeader();
  showWelcomeMessage();
  delay(2000);

  // Inisialisasi motor driver pin
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);

  // Inisialisasi pin LED
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // Inisialisasi pin sensor IR
  pinMode(SENSOR1, INPUT);
  pinMode(SENSOR2, INPUT);
  pinMode(SENSOR3, INPUT);
  pinMode(SENSOR4, INPUT);
  pinMode(SENSOR5, INPUT);

  showMainMenu();
}

void loop() {
  if (millis() - ledTimer > 500) {
    ledTimer = millis();
    ledState = !ledState;
    digitalWrite(LED1, ledState);
    digitalWrite(LED2, !ledState);
  }

  if (lineFollowerActive) {
    followLine();
    return;
  }

  if (IrReceiver.decode()) {
    uint32_t receivedCode = IrReceiver.decodedIRData.decodedRawData;
    if (receivedCode != lastReceivedCode || millis() - lastProcessedTime > 200) {
      lastReceivedCode = receivedCode;
      lastProcessedTime = millis();
      switch (currentMenu) {
        case 0:
          handleMainMenu(receivedCode);
          break;
        case 1:
          handleLineFollow(receivedCode);
          break;
        case 2:
          handleControlIR(receivedCode);
          break;
      }
    }
    IrReceiver.resume();
  }
}

void showWelcomeMessage() {
  u8x8.clearDisplay();
  const char* message = "Welcome";
  int pos = (16 - strlen(message)) / 2; 
  u8x8.drawString(pos, 1, message);
}

void showHeader() {
  u8x8.clearDisplay();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(4, 0, "| MENU |");
}

void showMainMenu() {
  showHeader();
  u8x8.drawString(0, 1, "1. Line Follow");
  u8x8.drawString(0, 2, "2. Control IR");
  selectedOption = 1;
}

void handleMainMenu(uint32_t code) {
  switch (code) {
    case 0xE718FF00:
      selectedOption = 1;
      u8x8.clearLine(1);
      u8x8.clearLine(2);
      u8x8.drawString(0, 1, ">1. Line Follow");
      u8x8.drawString(0, 2, "  2. Control IR");
      break;
    case 0xAD52FF00:
      selectedOption = 2;
      u8x8.clearLine(1);
      u8x8.clearLine(2);
      u8x8.drawString(0, 1, "  1. Line Follow");
      u8x8.drawString(0, 2, ">2. Control IR");
      break;
    case 0xE31CFF00:
      currentMenu = selectedOption;
      if (currentMenu == 1) {
        showLineFollowMenu();
      } else if (currentMenu == 2) {
        showControlIRMenu();
      }
      break;
  }
}

void showLineFollowMenu() {
  showHeader();
  u8x8.drawString(0, 1, "OK to Start");
  u8x8.drawString(0, 2, "* to Exit");
}

void handleLineFollow(uint32_t code) {
  if (code == 0xE31CFF00) {
    lineFollowerActive = !lineFollowerActive;
    if (lineFollowerActive) {
      u8x8.clearDisplay();
      u8x8.drawString(0, 1, "Running..");
    } else {
      stopMotors();
      showLineFollowMenu();
    }
  } else if (code == 0xE916FF00) {
    currentMenu = 0;
    showMainMenu();
  }
}

void showControlIRMenu() {
  showHeader();
  u8x8.drawString(0, 1, "Control IR Mode");
  u8x8.drawString(0, 2, "* to Exit");
}

void handleControlIR(uint32_t code) {
  switch (code) {
    case 0xE718FF00:
      moveForward();
      break;
    case 0xAD52FF00:
      moveBackward();
      break;
    case 0xA55AFF00:
      moveLeft();
      break;
    case 0xF708FF00:
      moveRight();
      break;
    case 0xE31CFF00:
      stopMotors();
      break;
    case 0xE916FF00:
      currentMenu = 0;
      showMainMenu();
      stopMotors();
      break;
  }
}

void followLine() {
  int sensor1 = digitalRead(SENSOR1); // Sensor paling kiri
  int sensor2 = digitalRead(SENSOR2); // Sensor kiri
  int sensor3 = digitalRead(SENSOR3); // Sensor tengah
  int sensor4 = digitalRead(SENSOR4); // Sensor kanan
  int sensor5 = digitalRead(SENSOR5); // Sensor paling kanan

  // Kecepatan dasar motor
  int speed = 130;

  // Logika pengendalian berdasarkan pembacaan sensor
  if (sensor3 == HIGH) {
    // Sensor tengah membaca garis, mobil bergerak lurus
    analogWrite(MOTOR_IN1, speed);
    analogWrite(MOTOR_IN3, speed);
  } else if (sensor2 == HIGH) {
    // Sensor kiri membaca garis, mobil sedikit ke kanan
    analogWrite(MOTOR_IN1, speed - 50);
    analogWrite(MOTOR_IN3, speed);
  } else if (sensor4 == HIGH) {
    // Sensor kanan membaca garis, mobil sedikit ke kiri
    analogWrite(MOTOR_IN1, speed);
    analogWrite(MOTOR_IN3, speed - 50);
  } else if (sensor1 == HIGH) {
    // Sensor paling kiri membaca garis, tikungan tajam ke kanan
    analogWrite(MOTOR_IN1, speed - 100);
    analogWrite(MOTOR_IN3, speed);
  } else if (sensor5 == HIGH) {
    // Sensor paling kanan membaca garis, tikungan tajam ke kiri
    analogWrite(MOTOR_IN1, speed);
    analogWrite(MOTOR_IN3, speed - 100);
  } else {
    // Tidak ada garis terdeteksi, berhenti untuk keamanan
    stopMotors();
  }

  // Logika tambahan untuk tikungan tajam
  if (sensor3 == HIGH && sensor4 == HIGH && sensor5 == HIGH) {
    // Tikungan tajam ke kanan, fokus pada garis di kanan
    analogWrite(MOTOR_IN1, speed);
    analogWrite(MOTOR_IN3, 0);
  } else if (sensor3 == HIGH && sensor2 == HIGH && sensor1 == HIGH) {
    // Tikungan tajam ke kiri, fokus pada garis di kiri
    analogWrite(MOTOR_IN1, 0);
    analogWrite(MOTOR_IN3, speed);
  }
}


void moveForward() {
  analogWrite(MOTOR_IN1, 150);
  analogWrite(MOTOR_IN2, 0);
  analogWrite(MOTOR_IN3, 150);
  analogWrite(MOTOR_IN4, 0);
}

void moveBackward() {
  analogWrite(MOTOR_IN1, 0);
  analogWrite(MOTOR_IN2, 150);
  analogWrite(MOTOR_IN3, 0);
  analogWrite(MOTOR_IN4, 150);
}

void moveLeft() {
  analogWrite(MOTOR_IN1, 0);
  analogWrite(MOTOR_IN2, 150);
  analogWrite(MOTOR_IN3, 150);
  analogWrite(MOTOR_IN4, 0);
}

void moveRight() {
  analogWrite(MOTOR_IN1, 150);
  analogWrite(MOTOR_IN2, 0);
  analogWrite(MOTOR_IN3, 0);
  analogWrite(MOTOR_IN4, 150);
}

void stopMotors() {
  analogWrite(MOTOR_IN1, 0);
  analogWrite(MOTOR_IN2, 0);
  analogWrite(MOTOR_IN3, 0);
  analogWrite(MOTOR_IN4, 0);
}
