// ==========================================
// NODO RECEPTOR - SISTEMA DE SEGURIDAD CCTV
// Verificación de Integridad por CRC-8 (LFSR)
// ==========================================

#include <HardwareSerial.h>

// Interfaz UART Física (Alineada con el Emisor)
HardwareSerial PuertoUART(1);
const int PIN_RX = 20;
const int PIN_TX = 21;

// Pines de Interfaz Física (LEDs de Diagnóstico)
const int LED_ROJO = 3;   // Error / Trama Corrupta
const int LED_VERDE = 4;  // Éxito / Integridad Validada

// Buffer RAM de Recepción
const int TAMANO_TRAMA = 16;
uint8_t buffer_ram[TAMANO_TRAMA];

void setup() {
  // Inicialización de Monitor Serie para Debugging en PC
  Serial.begin(115200);
  
  // Inicialización del Bus UART (9600 baudios, 8 bits de datos, sin paridad, 1 bit de parada)
  PuertoUART.begin(9600, SERIAL_8N1, PIN_RX, PIN_TX);
  
  // Configuración de Pines
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  
  // Estado inicial: LEDs apagados
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_VERDE, LOW);

  Serial.println("====================================");
  Serial.println("NODO RECEPTOR INICIADO - ESP32-C3");
  Serial.println("Esperando tramas UART...");
  Serial.println("====================================");
}

// Subrutina CRC-8 (Motor LFSR - Polinomio 0x07)
// Debe ser matemáticamente idéntico al del Emisor
uint8_t calc_crc8(uint8_t d[], uint8_t size) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < size; i++) {
    crc ^= d[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc; // Retorna el residuo temporal
}

void loop() {
  // MÁQUINA DE ESTADOS: FASE DE ESCRITURA EN RAM
  // Espera a que el decodificador reciba exactamente 16 Bytes
  if (PuertoUART.available() >= TAMANO_TRAMA) {
    
    // Almacena los 16 Bytes en el Buffer RAM
    PuertoUART.readBytes(buffer_ram, TAMANO_TRAMA);
    
    Serial.println("\n[+] TRAMA DE 16 BYTES INTERCEPTADA");

    // MÁQUINA DE ESTADOS: FASE DE VERIFICACIÓN
    // 1. Extraemos la firma original que llegó en el Byte 16 (Índice 15)
    uint8_t crc_recibido = buffer_ram[15];
    
    // 2. Extraemos el Payload (15 Bytes) y recalculamos el CRC localmente
    uint8_t crc_calculado = calc_crc8(buffer_ram, 15);
    
    // 3. Comparador Digital Lógico
    Serial.print("CRC Original (Recibido): 0x");
    Serial.println(crc_recibido, HEX);
    Serial.print("CRC Nuevo (Calculado)  : 0x");
    Serial.println(crc_calculado, HEX);

    if (crc_calculado == crc_recibido) {
      // Veredicto: Integridad 100%
      Serial.println(">> RESULTADO: MATCH EXACTO. ACCION AUTORIZADA.");
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_ROJO, LOW);
      
      // Aquí se enviaría la orden lógica final a la cámara IP
      if (buffer_ram[2] == 1) {
        Serial.println("ORDEN: INICIANDO GRABACION DEL INTRUSO.");
      } else {
        Serial.println("ORDEN: MANTENER REPOSO.");
      }

    } else {
      // Veredicto: Divergencia Matemática (Ruido en el canal)
      Serial.println(">> RESULTADO: DIVERGENCIA DETECTADA! (ERROR DE CANAL)");
      Serial.println("ACCION: TRAMA DESCARTADA. BLOQUEANDO ORDEN.");
      digitalWrite(LED_ROJO, HIGH);
      digitalWrite(LED_VERDE, LOW);
    }

    // Mantenemos el estado visual por 2 segundos para la demostración
    delay(2000);
    
    // Retorno al estado de reposo, limpiando los registros visuales
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_ROJO, LOW);
    
    // Limpieza del buffer UART físico por si llegó ruido extra o rebotes
    while(PuertoUART.available() > 0) {
      PuertoUART.read();
    }
    
    Serial.println("\nEsperando nueva trama...");
  }
}