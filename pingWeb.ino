#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <TinyGPS.h>

#include <ArduinoJson.h>


//RFID
#define SS_PIN 10 //RFID
#define RST_PIN 9 //RFID
//GPS
#define GPS_RX 4 //GPS
#define GPS_TX 3 //GPS
TinyGPS gps;  //GPS
SoftwareSerial gpsSerial(GPS_RX, GPS_TX); //GPS
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

//WIFI
SoftwareSerial esp8266(6,7);
#define DEBUG true
//--------------------

int code[] = {32,154,149,117}; //This is the stored UID (Unlock Card)

int codeRead = 0;

String uidString;

char appData;

String inData = "";

String Local1 = "163 110 28 174";
String Local2 = "99 240 138 152";


//Função WIFI
String sendData(String command, const int timeout, boolean debug)
{
  // Envio dos comandos AT para o modulo
  String response = "";
  esp8266.print(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (esp8266.available())
    {
      // The esp has data so display its output to the serial window
      char c = esp8266.read(); // read the next character.
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
  }
  return response;
}

void setup()
{ 
  Serial.begin(9600);

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  // Serial.println(F("Arduino RFID tutorial")); //RFID

  gpsSerial.begin(9600);
//WIFI
  esp8266.begin(19200);

  sendData("AT+RST\r\n", 2000, DEBUG); // rst
  // Conecta a rede wireless
  sendData("AT+CWJAP=\"tony1\",\"12345678\"\r\n", 2000, DEBUG);
  delay(3000);
  sendData("AT+CWMODE=1\r\n", 1000, DEBUG);
  // Mostra o endereco IP
  sendData("AT+CIFSR\r\n", 1000, DEBUG);
  // Configura para multiplas conexoes
  sendData("AT+CIPMUX=1\r\n", 1000, DEBUG);
  // Inicia o web server na porta 80
  sendData("AT+CIPSERVER=1,80\r\n", 1000, DEBUG);
}

void loop()
{
  
  //===========================================GPS============================
  bool newData = false;
  unsigned long chars;
  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (gpsSerial.available())
    {
      char c = gpsSerial.read();
//       Serial.write(c); //apague o comentario para mostrar os dados crus
      if (gps.encode(c)) // Atribui true para newData caso novos dados sejam recebidos
        newData = true;
    }
  }
  /*if (newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    Serial.println();
    Serial.print("lat=");
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print(" lon=");
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);  
    newData = false;
  }*/

  if (esp8266.available())
  {
    if (esp8266.find("+IPD,"))
    {
      delay(300);
      int connectionId = esp8266.read() - 48;


//GPS
      float flat, flon;
      unsigned long age;
      gps.f_get_position(&flat, &flon, &age);
      
//========Json===========
      StaticJsonDocument<100> testDocument;
      testDocument["name"] = "NULL";
      testDocument["lat"] = flat;
      testDocument["lon"] = flon;
      char buffer[100];
      serializeJsonPretty(testDocument, buffer);
//=============WebPage====================
      String webpage = "<head><meta http-equiv=""refresh"" content=""3"">";
      webpage += "</head><h1><u>TESTANDO </u></h1><h2>";
      webpage += buffer;     
      webpage += "</h2>";
 
      String cipSend = "AT+CIPSEND=";
      cipSend += connectionId;
      cipSend += ",";
      cipSend += webpage.length();
      cipSend += "\r\n";
 
      sendData(cipSend, 1000, DEBUG);
      sendData(webpage, 1000, DEBUG);
 
      String closeCommand = "AT+CIPCLOSE=";
      closeCommand += connectionId; // append connection id
      closeCommand += "\r\n";
 
      sendData(closeCommand, 3000, DEBUG);
    }
   }

 //========================================RFID===================================
  if(  rfid.PICC_IsNewCardPresent())
  {
      readRFID();

      
      delay(5000);
  }
  delay(100);
}

void readRFID()
{

  rfid.PICC_ReadCardSerial();
  // tipo de tag Serial.print(F("\nPICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // tipo de tag Serial.println(rfid.PICC_GetTypeName(piccType));
  // Check is the PICC of Classic MIFARE type

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {

    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }
    // Mostra o tag Serial.println("Scanned PICC's UID:");
    // mostra o trag printDec(rfid.uid.uidByte, rfid.uid.size);
    uidString = String(rfid.uid.uidByte[0])+" "+String(rfid.uid.uidByte[1])+" "+String(rfid.uid.uidByte[2])+ " "+String(rfid.uid.uidByte[3]);
    int i = 0;
    boolean match = true;
    while(i<rfid.uid.size)
    {
      if(!(int(rfid.uid.uidByte[i]) == int(code[i])))
      {
           match = false;   
      }
      i++;
    }
        if(Local1 == uidString)
        {
          Serial.println();

          float flat, flon;
          unsigned long age;
          gps.f_get_position(&flat, &flon, &age);
          StaticJsonDocument<100> testDocument;
          testDocument["name"] = "Uniso - Bloco F";
          testDocument["lat"] = flat;
          testDocument["lon"] = flon;
          char buffer[100];
          serializeJsonPretty(testDocument, buffer);
          Serial.print(buffer);

          if (esp8266.available())
           {
             if (esp8266.find("+IPD,"))
              {
                delay(300);
                int connectionId = esp8266.read() - 48;
                //GPS
                float flat, flon;
                unsigned long age;
                gps.f_get_position(&flat, &flon, &age);
      
                //========Json===========
                StaticJsonDocument<100> testDocument;
                testDocument["name"] = "Uniso - Bloco F";
                testDocument["lat"] = flat;
                testDocument["lon"] = flon;
                char buffer[100];
                serializeJsonPretty(testDocument, buffer);
          //=============WebPage====================
                String webpage = "<head><meta http-equiv=""refresh"" content=""3"">";
                webpage += "</head><h1><u>TESTANDO </u></h1><h2>";
                webpage += buffer;     
                webpage += "</h2>";
           
                String cipSend = "AT+CIPSEND=";
                cipSend += connectionId;
                cipSend += ",";
                cipSend += webpage.length();
                cipSend += "\r\n";
           
                sendData(cipSend, 1000, DEBUG);
                sendData(webpage, 1000, DEBUG);
           
                String closeCommand = "AT+CIPCLOSE=";
                closeCommand += connectionId; // append connection id
                closeCommand += "\r\n";
           
                sendData(closeCommand, 3000, DEBUG);
              }
             }
        }
        if(Local2 == uidString)
        {
          Serial.println();
          float flat, flon;
          unsigned long age;
          gps.f_get_position(&flat, &flon, &age);
          StaticJsonDocument<100> testDocument;
          testDocument["name"] = "Uniso - Apoio 1";
          testDocument["lat"] = flat;
          testDocument["lon"] = flon;
          char buffer[100];
          serializeJsonPretty(testDocument, buffer);
          Serial.print(buffer);

          if (esp8266.available())
           {
             if (esp8266.find("+IPD,"))
              {
                delay(300);
                int connectionId = esp8266.read() - 48;
                //GPS
                float flat, flon;
                unsigned long age;
                gps.f_get_position(&flat, &flon, &age);
      
                //========Json===========
                StaticJsonDocument<100> testDocument;
                testDocument["name"] = "Uniso - Apoio 1";
                testDocument["lat"] = flat;
                testDocument["lon"] = flon;
                char buffer[100];
                serializeJsonPretty(testDocument, buffer);
          //=============WebPage====================
                String webpage = "<head><meta http-equiv=""refresh"" content=""3"">";
                webpage += "</head><h1><u>TESTANDO </u></h1><h2>";
                webpage += buffer;     
                webpage += "</h2>";
           
                String cipSend = "AT+CIPSEND=";
                cipSend += connectionId;
                cipSend += ",";
                cipSend += webpage.length();
                cipSend += "\r\n";
           
                sendData(cipSend, 1000, DEBUG);
                sendData(webpage, 1000, DEBUG);
           
                String closeCommand = "AT+CIPCLOSE=";
                closeCommand += connectionId; // append connection id
                closeCommand += "\r\n";
           
                sendData(closeCommand, 3000, DEBUG);
              }
             }
        }
       // Serial.println("\nUnknown Card");
    
   // espaço Serial.println("============================");
    // Halt PICC

  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}


/*
void printDec(byte *buffer, byte bufferSize) {

  for (byte i = 0; i < bufferSize; i++)
   {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
*/
