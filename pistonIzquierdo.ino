#include <Wire.h>

#define MOD_ADDR          0x01       // Dirección I2C para el esclavo izquierdo
#define RELAY_NUM         6
#define TIME_ON_DELAY     2000       // Tiempo que los pistones permanecen activos
#define TIME_OFF_DELAY    500        // Tiempo de espera antes de activar los siguientes pistones
#define GAME_DURATION     30000      // 30 segundos
#define FORCE_THRESHOLD   200        // Umbral para detectar fuerza (ajusta según sea necesario)

// Variables globales
uint8_t output_pin[RELAY_NUM] = {9, 8, 7, 6, 5, 4}; // Pines conectados a pistones
volatile int leftScore = 0;
bool game_active = false;
bool forceRegistered = false;         // Bandera para detectar golpe del sensor 
unsigned long lastChangeTime = 0;
unsigned long game_end_time = 0;
unsigned long interval = TIME_ON_DELAY + TIME_OFF_DELAY;

const int numSensors = 6;        // Número de sensores
const int fsrPins[numSensors] = {A0, A1, A2, A3, A4, A5}; // Pines de los sensores

int fsrReadings[numSensors];    // Array para almacenar las lecturas de los sensores
int counters[numSensors] = {0}; // Array para contar las detecciones de golpes
bool hitDetected[numSensors] = {false}; // Array para rastrear si se ha detectado un golpe

void setup() {
  Wire.begin(MOD_ADDR);          // Inicializa el dispositivo como esclavo I2C con dirección 0x01
  Wire.onRequest(requestEvent);  // Función llamada cuando el maestro solicita datos
  Wire.onReceive(receiveEvent);  // Función llamada cuando el maestro envía datos

  // Apaga todos los pistones (polaridad invertida)
  for (int i = 0; i < RELAY_NUM; i++) {
    pinMode(output_pin[i], OUTPUT);
    digitalWrite(output_pin[i], HIGH);
  }

  Serial.begin(9600);
  randomSeed(analogRead(0));     // Inicializa el generador de números aleatorios
}

void loop() {
  if (game_active) {
    unsigned long currentTime = millis();

    if (currentTime - lastChangeTime >= interval) {
      lastChangeTime = currentTime;
      activateRandomRelays();

      // Valida si el juego sigue activo
      if (currentTime >= game_end_time) {
        stopAllRelays();
        game_active = false;
      }
    }

    // Procesar las lecturas de los sensores
    for (int i = 0; i < numSensors; i++) {
      fsrReadings[i] = analogRead(fsrPins[i]); // Leer el valor del sensor

      if (fsrReadings[i] > FORCE_THRESHOLD) {
        if (!hitDetected[i]) { // Si no se ha detectado un golpe previamente en este sensor
          counters[i]++;
          Serial.print("Sensor ");
          Serial.print(i);
          Serial.print(": Count: ");
          Serial.print(counters[i]);
          hitDetected[i] = true; // Marca que se ha detectado un golpe en este sensor
          leftScore = counters[i]; // Actualiza el puntaje
        }
      } else {
        if (hitDetected[i]) { // Si el golpe ha sido detectado y ahora no se detecta
          hitDetected[i] = false; // Resetea el estado del golpe
          Serial.println(" - NO HIT");
        } else {
          Serial.println(" - NO HIT");
        }
      }
    }
    delay(100); // Espera un poco antes de la próxima lectura
  }
}

void activateRandomRelays() {
  stopAllRelays();

  // Activa dos pistones aleatorios
  int relay1 = random(0, RELAY_NUM);
  int relay2;

  do {
    relay2 = random(0, RELAY_NUM);
  } while (relay2 == relay1);

  digitalWrite(output_pin[relay1], LOW);
  digitalWrite(output_pin[relay2], LOW);
}

void stopAllRelays() {
  for (int i = 0; i < RELAY_NUM; i++) {
    digitalWrite(output_pin[i], HIGH); 
  }
}

void requestEvent() {
  Wire.write(leftScore & 0xFF);         // Enviar el puntaje bajo 8 bits
  Wire.write((leftScore >> 8) & 0xFF);  // Enviar el puntaje alto 8 bits
}

void receiveEvent(int howMany) {
  if (Wire.available() > 0) {
    byte command = Wire.read();

    if (command == 0x01) { // Comando para iniciar la activación aleatoria de pistones
      game_end_time = millis() + GAME_DURATION;
      game_active = true; // Inicia el juego
      lastChangeTime = millis(); // Reinicia el temporizador de activación de pistones
      Serial.println("Inicio del juego recibido");
    } else if (command == 0xFF) { // Comando para detener todos los pistones
      stopAllRelays();
      game_active = false; // Detiene el juego
    }
  }
}
