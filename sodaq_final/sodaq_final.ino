#include <Sodaq_RN2483.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define debugSerial SerialUSB
#define loraSerial Serial2

// BME280
Adafruit_BME280 bme; // I2C

// MQ-9B Analog Pin
const int MQ9B_PIN = A1;

// OTAA
//6081F9EED20F7C48
uint8_t DevEUI[8] =
{
  0x60, 0x81, 0xF9, 0xEE, 0xD2, 0x0F, 0x7C, 0x48
};

//6081F9464568E46F
const uint8_t AppEUI[8] =
{
  0x60, 0x81, 0xF9, 0x46, 0x45, 0x68, 0xE4, 0x6F
};

// 2B540692403DCA05C5E6B8F878D523BF
const uint8_t AppKey[16] =
{
0x2B, 0x54, 0x06, 0x92, 0x40, 0x3D, 0xCA, 0x05,
0xC5, 0xE6, 0xB8, 0xF8, 0x78, 0xD5, 0x23, 0xBF
};

void setup()
{
  SerialUSB.begin(57600);

  while (!SerialUSB);

  debugSerial.println("Start");

  // Start streams
  debugSerial.begin(57600);
  loraSerial.begin(LoRaBee.getDefaultBaudRate());

  LoRaBee.setDiag(SerialUSB); // to use debug remove //DEBUG inside library
  //LoRaBee.init(loraSerial, LORA_RESET);
  if (LoRaBee.initOTA(loraSerial, DevEUI, AppEUI, AppKey, true))
  {
    SerialUSB.println("Network connection successful.");
  }
  else
  {
    SerialUSB.println("Network connection failed!");
  }

  debugSerial.println();  

  if (!bme.begin(0x76)) { // Remplacez 0x76 par l'adresse I2C de votre BME280 si différente
    debugSerial.println("Could not find a valid BME280 sensor, check wiring!");
  }

}


void loop()
{
  String temperature = getTemperature();
  String humidity = String(bme.readHumidity());
  String pressure = String(bme.readPressure() / 100.0F); // La pression est en Pa, conversion en hPa
  int mq9Value = analogRead(MQ9B_PIN); // Lecture du capteur MQ-9B


  String reading = temperature + "," + humidity + "," + pressure + "," + String(mq9Value);
   
  debugSerial.println(reading);

  const char* readingBytes = reading.c_str();
  size_t length = reading.length();

  debugSerial.print("Bytes: ");
  for (size_t i = 0; i < length; i++) {
    debugSerial.print((uint8_t)readingBytes[i], HEX); // Imprimer en hexadécimal
    debugSerial.print(" "); // Espace entre les bytes pour la lisibilité
  }
  debugSerial.println();
  
  switch (LoRaBee.send(1, (uint8_t*)reading.c_str(), reading.length()))
  {
    case NoError:
      debugSerial.println("Successful transmission.");
      break;
    case NoResponse:
      debugSerial.println("No Response");
      break;
    case Timeout:
      debugSerial.println("Time out");
      break;
    case PayloadSizeError:
      debugSerial.println("Payload Size Error");
      break;
    case InternalError:
      debugSerial.println("Internal Error");
      break;
    case Busy:
      debugSerial.println("Busy, attempting to reset LoRa module and retry...");
      // Réinitialisation de la connexion LoRaBee
      if (LoRaBee.initOTA(loraSerial, DevEUI, AppEUI, AppKey, true)) {
        debugSerial.println("Reconnected to the network. Retrying transmission...");
        // Tentative de renvoi des données
        if (LoRaBee.send(1, (uint8_t*)reading.c_str(), reading.length()) == NoError) {
          debugSerial.println("Successful transmission after retry.");
        } else {
          debugSerial.println("Failed to transmit after retry.");
        }
      } else {
        debugSerial.println("Failed to reconnect to the network.");
      }
      break;
    case NetworkFatalError:
      debugSerial.println("Network Fatal Error");
      break;
    case NotConnected:
      debugSerial.println("The device is not connected to the network. The program will reset the RN module.");
      LoRaBee.initOTA(loraSerial, DevEUI, AppEUI, AppKey, true);
      break;
    case NoAcknowledgment:
      debugSerial.println("No Acknowledgment");
      break;
    default:
      debugSerial.println("Other");
      break;
  }
    // Delay between readings
    // 60 000 = 1 minute
  delay(10000); 
}

String getTemperature()
{
  //10mV per C, 0C is 500mV
  float mVolts = (float)analogRead(TEMP_SENSOR) * 3300.0 / 1023.0;
  float temp = (mVolts - 500.0) / 10.0;

  return String(temp);
}

