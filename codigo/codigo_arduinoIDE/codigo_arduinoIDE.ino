#include <SoftwareSerial.h>
#include <DHT.h>
#include <Keypad.h>
#include <Servo.h>

#define DHTPIN 5      // Pin donde está conectado el sensor DHT
#define DHTTYPE DHT11 // Tipo de sensor DHT (DHT11 o DHT22)
#define BUZZER_PIN 6  // Pin donde está conectado el buzzer

SoftwareSerial BTSerial(10, 11); // RX, TX

const int ledPin1 = 2; // Pin donde está conectado el primer LED
const int ledPin2 = 3; // Pin donde está conectado el segundo LED
const int ledPin3 = 4; // Pin donde está conectado el tercer LED

DHT dht(DHTPIN, DHTTYPE);
Servo myServo; // Objeto del servo para la tranquera
Servo doorServo; // Objeto del servo para la puerta

// Configuración del teclado numérico
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {A1, A2, A3, A4};
byte colPins[COLS] = {A5, 9, 12, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const float FIRE_TEMP_THRESHOLD = 28.0; // Umbral de temperatura para detectar incendio

// Contraseña esperada
String password = "1234";
String inputPassword = ""; // Variable para la contraseña ingresada
int incorrectAttempts = 0; // Contador de intentos incorrectos

unsigned long lastPrintTime = 0; // Variable para almacenar el último tiempo de impresión
const unsigned long printInterval = 4000; // Intervalo de impresión en milisegundos

void setup() {
  pinMode(ledPin1, OUTPUT); // Configurar el pin del primer LED como salida
  pinMode(ledPin2, OUTPUT); // Configurar el pin del segundo LED como salida
  pinMode(ledPin3, OUTPUT); // Configurar el pin del tercer LED como salida
  pinMode(BUZZER_PIN, OUTPUT); // Configurar el pin del buzzer como salida

  BTSerial.begin(9600); // Inicializar comunicación con el módulo Bluetooth
  Serial.begin(9600); // Inicializar comunicación serial para depuración

  dht.begin(); // Inicializar el sensor DHT
  myServo.attach(8); // Pin de control del servo SG90 para la tranquera
  doorServo.attach(7); // Pin de control del servo SG90 para la puerta
}

void loop() {
  unsigned long currentTime = millis(); // Obtener el tiempo actual

  // Leer datos del sensor DHT
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Comprobar si hay fallo en la lectura
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Error leyendo del sensor DHT");
    return;
  }

  // // Mostrar los valores leídos en el monitor serial cada 4 segundos
  // if (currentTime - lastPrintTime >= printInterval) {
  //   lastPrintTime = currentTime;
  //   Serial.print("Humedad: ");
  //   Serial.print(humidity);
  //   Serial.print(" %\t");
  //   Serial.print("Temperatura: ");
  //   Serial.print(temperature);
  //   Serial.println(" *C");
  // }

  // Comprobar si la temperatura excede el umbral de incendio
  if (temperature > FIRE_TEMP_THRESHOLD) {
    BTSerial.println("Alerta: Incendio detectado!");
    Serial.println("Alerta: Incendio detectado!");
    digitalWrite(BUZZER_PIN, HIGH); // Activar buzzer
    sendEventToServer("Alerta: Incendio detectado!"); // Enviar alerta al servidor
  } else {
    digitalWrite(BUZZER_PIN, LOW); // Desactivar buzzer
  }

  // Leer comandos Bluetooth
  if (BTSerial.available()) { // Verificar si hay datos disponibles desde el Bluetooth
    char command = BTSerial.read(); // Leer el comando recibido
    Serial.println(command); // Mostrar el comando para depuración

    if (command == '1') { // Si el comando es '1'
      digitalWrite(ledPin1, HIGH); // Prender el primer LED
      sendEventToServer("LED1 ON");
    } else if (command == '0') { // Si el comando es '0'
      digitalWrite(ledPin1, LOW); // Apagar el primer LED
      sendEventToServer("LED1 OFF");
    } else if (command == '2') { // Si el comando es '2'
      digitalWrite(ledPin2, HIGH); // Prender el segundo LED
      sendEventToServer("LED2 ON");
    } else if (command == '3') { // Si el comando es '3'
      digitalWrite(ledPin2, LOW); // Apagar el segundo LED
      sendEventToServer("LED2 OFF");
    } else if (command == '4') { // Si el comando es '4'
      digitalWrite(ledPin3, HIGH); // Prender el tercer LED
      sendEventToServer("LED3 ON");
    } else if (command == '5') { // Si el comando es '5'
      digitalWrite(ledPin3, LOW); // Apagar el tercer LED
      sendEventToServer("LED3 OFF");
    } else if (command == '6') { // Si el comando es '6'
      openGate(); // Abrir la tranquera
      sendEventToServer("Tranquera abierta");
    } else if (command == '7') { // Si el comando es '7'
      closeGate(); // Cerrar la tranquera
      sendEventToServer("Tranquera cerrada");
    }
  }

  // Leer entrada del teclado numérico
  char key = keypad.getKey();
  
  if (key != NO_KEY) {
    // Procesar la entrada del teclado
    if (isDigit(key)) {
      inputPassword += key; // Concatenar el dígito a la contraseña ingresada
    } else if (key == '#') {
      checkPassword(); // Verificar la contraseña al presionar #
    }
  }

  delay(200); // Pequeño delay para evitar lecturas rápidas del teclado
}

// Función para verificar la contraseña ingresada
void checkPassword() {
  if (inputPassword == password) {
    BTSerial.println("Contraseña correcta!");
    Serial.println("Contraseña correcta!");
    openDoor(); // Abrir la puerta al ingresar la contraseña correcta
    sendEventToServer("Contraseña correcta. Puerta abierta.");
    inputPassword = ""; // Reiniciar la variable de entrada de contraseña
    incorrectAttempts = 0; // Reiniciar el contador de intentos incorrectos
  } else {
    BTSerial.println("Alerta: Contraseña incorrecta! Posible robo detectado.");
    Serial.println("Alerta: Contraseña incorrecta! Posible robo detectado.");
    sendEventToServer("Contraseña incorrecta. Posible robo detectado.");
    incorrectAttempts++; // Incrementar el contador de intentos incorrectos
    if (incorrectAttempts >= 3) {
      digitalWrite(BUZZER_PIN, HIGH); // Activar buzzer
      delay(5000); // Mantener el buzzer activado por 5 segundos
      digitalWrite(BUZZER_PIN, LOW); // Desactivar buzzer
      sendEventToServer("Alarma activada por intentos incorrectos.");
    }
    inputPassword = ""; // Reiniciar la variable de entrada de contraseña
  }
}

// Función para abrir la puerta
void openDoor() {
  doorServo.write(90); // Ángulo de 90 grados (posición abierta)
  Serial.println("Puerta abierta");
  delay(5000); // Mantener la puerta abierta por 5 segundos (ajusta el tiempo según tus necesidades)
  closeDoor(); // Cerrar la puerta después del tiempo
  sendEventToServer("Puerta cerrada.");
}

// Función para cerrar la puerta
void closeDoor() {
  doorServo.write(0); // Ángulo de 0 grados (posición cerrada)
  Serial.println("Puerta cerrada");
}

// Función para abrir la tranquera
void openGate() {
  myServo.write(90); // Ángulo de 90 grados (posición abierta)
  Serial.println("Tranquera abierta");
}

// Función para cerrar la tranquera
void closeGate() {
  myServo.write(0); // Ángulo de 0 grados (posición cerrada)
  Serial.println("Tranquera cerrada");
}

// Función para enviar eventos al servidor
void sendEventToServer(String event) {
  Serial.println(event);
}