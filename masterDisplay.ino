#include <DMD2.h>
#include <EEPROM.h>
#include <Wire.h>
#include <fonts/Arial_Black_16.h>

#define HEIGTH_DISPLAY 2
#define WIDTH_DISPLAY 1
#define SLAVE_LEFT_ADDR 0x01        // Dirección I2C del esclavo izquierdo
#define SLAVE_RIGHT_ADDR 0x02       // Dirección I2C del esclavo derecho
#define GAME_DURATION 30000         // Duración del juego en milisegundos
#define SCROLL_DELAY 100            // Tiempo entre cada desplazamiento del texto TODO: Ver si acelerar el tiempo
#define BUTTON_PIN  A0              // Boton para iniciar el juego (mantener presionado)
#define LONG_PRESS_TIME 3000        // Tiempo en milisegundos para considerar una presion larga
#define TIME_BETWEEN_COUNTDOWN_NUMBERS 1000   //Tiempo en milisegundos para la cuenta regresiva 
#define SCORE_TIME 8000             // Tiempo en milisegundos para mostrar el score

uint32_t timer;
int dmd_x = 64; // Numero total de leds a lo ancho

SoftDMD dmd(HEIGTH_DISPLAY, WIDTH_DISPLAY);
byte Brightness;
int rightScore = 0;
int leftScore = 0;
char dmdBuff[10];
uint32_t game_start_time = 0;
bool game_active = false;
bool countdown_active = false;

void setup() {
  Brightness = EEPROM.read(0);
  dmd.setBrightness(10);
  dmd.selectFont(Arial_Black_16);
  dmd.begin();
  dmd.clearScreen();
  Serial.begin(9600);
  Wire.begin();
  
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura boton para inicio
  
  displayAndHandleReadyMessage(); //Muestra mensaje inicial ("Ready!")
}

void loop() {
  if (countdown_active) {
    countdown();
  } else if (game_active && (millis() - game_start_time >= GAME_DURATION)) { //Si el juego esta activo y si ha pasado "GAME_DURATION"
    game_active = false; 
    dmd.clearScreen();
    if (leftScore > rightScore) {
      dmd.drawString(0, 0, "WIN");
    } else if (rightScore > leftScore) {
      dmd.drawString(32, 0, "WIN");
    } else {
      dmd.drawString(0, 0, "DRAW");
      dmd.drawString(32, 0, "DRAW");
    }
    delay(SCORE_TIME); // Mostrar resultado por "SCORE_TIME"

    // Notifica al esclavo derecho que el juego ha terminado
    Wire.beginTransmission(SLAVE_RIGHT_ADDR);
    Wire.write(0xFF); // Comando para detener todos los pistones
    Wire.endTransmission();
    
    return; // Fin del juego
  } else if (!countdown_active && !game_active) { //El juego no esta activo
    displayAndHandleReadyMessage();
  } else if (game_active) { //Si el juego esta activo
    // Solicita y muestra los puntajes de los esclavos mientras el juego esta activo
    Wire.requestFrom(SLAVE_LEFT_ADDR, 2); // Leer puntaje del esclavo izquierdo
    if (Wire.available() == 2) {
      leftScore = Wire.read() | (Wire.read() << 8);
    }

    Wire.requestFrom(SLAVE_RIGHT_ADDR, 2); // Leer puntaje del esclavo derecho
    if (Wire.available() == 2) {
      rightScore = Wire.read() | (Wire.read() << 8);
    }

    dmd.clearScreen();
    sprintf(dmdBuff, "%d", leftScore);
    dmd.drawString(0, 0, dmdBuff);
    dmd.drawString(30, 0, "-");
    sprintf(dmdBuff, "%2d", rightScore);
    dmd.drawString(43, 0, dmdBuff);
    delay(300); // Actualizar cada 300 ms
  }
}

void displayAndHandleReadyMessage() {
  uint32_t pressStartTime = 0; // Variable para controlar el tiempo de presion del boton

  while (true) {
    dmd.clearScreen();
    dmd.drawString(dmd_x, 1, F("Ready!"));
    
    // Mueve el texto de derecha a izquierda
    if (dmd_x <= -64) {
      dmd_x = 64;
    } else {
      dmd_x--;
    }
    
    // Si es presionado el boton
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (pressStartTime == 0) {
        pressStartTime = millis(); // Inicia el temporizador cuando el boton se presiona
      } else if (millis() - pressStartTime >= LONG_PRESS_TIME) {
        // Si el boton ha sido presionado por LONG_PRESS_TIME, comienza la cuenta regresiva
        countdown_active = true;
        timer = millis(); // Reinicia el temporizador
        dmd.clearScreen();
        return;
      }
    } else {
      pressStartTime = 0; // Resetea el temporizador si el boton no esta siendo presionado
    }
    delay(SCROLL_DELAY); // Controla la velocidad del texto
  }
}

void countdown() {
  for (uint8_t i = 3; i > 0; i--) {
    dmd.clearScreen();
    dmd.drawString(10, 1, String(i));
    dmd.drawString(35, 1, String(i));
    delay(TIME_BETWEEN_COUNTDOWN_NUMBERS); // 1 segundo entre numeros
  }
  
  dmd.clearScreen();
  dmd.drawString(10, 1, F("GO!"));
  dmd.drawString(40, 1, F("GO!"));
  delay(1000); // Muestra "GO!" por 1 segundo
  
  dmd.clearScreen(); // Limpia la pantalla antes de iniciar el marcador
  startGame(); // Inicia el juego despues de la cuenta regresiva
}

void startGame() {
  game_active = true;
  game_start_time = millis();
  
  // Notifica al esclavo derecho que el juego ha comenzado
  Wire.beginTransmission(SLAVE_RIGHT_ADDR);
  Wire.write(0x01); // Comando para activar los pistones de forma aleatoria
  Wire.endTransmission();

  // Notifica al esclavo izquierdo que el juego ha comenzado
  Wire.beginTransmission(SLAVE_LEFT_ADDR);
  Wire.write(0x01); // Comando para activar los pistones de forma aleatoria
  Wire.endTransmission();
  
  countdown_active = false;
}


