/*
HELLO GUYS THIS CODE is auto miner, made with pumafron afk, the code mine only using 100% arduino and ethernet shield
thannks you LDarki for help me to fix connection error
the proyect is arduino miner to dunicoin made with revox
thanks you Joybed to fix the hashrate problem
*/

//#define __DEBUG__ //enables or disables serial console, disabling it may result in higher hashrates, uncomment if you want serial enabled
//#define __MANUAL_POOL


#pragma GCC optimize ("-Ofast")

#ifndef LED_BUILTIN
#define LED_BUILTIN 13

#endif
/* For 8-bit microcontrollers we should use 16 bit variables since the
difficulty is low, for all the other cases should be 32 bits. */
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
typedef uint16_t uintDiff;
#else
typedef uint32_t uintDiff;
#endif
// Arduino identifier library - https://github.com/ricaun
#include "uniqueID.h"
#include "duco_hash.h"
#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// the pool ip final 
// http://server.duinocoin.com/getPool
byte pool[] = {0,0,0,0};
unsigned short port = 0;

//miner global variables
String Username = "Puma"; //put your username here
const char* RIG_IDENTIFIER = "None"; //put your rig identifier here
String key = "None";


String DUCOID = "";

uintDiff ducos1result = 0;

const uint16_t job_maxsize = 104;
uint8_t job[job_maxsize];

//client variables SETTINGS
const char * miner_version = "Pumitahou 3.0";
String VER = "3.0";
String start_diff = "AVR";

String SEPARATOR = ",";
String BLOCK = " ‖ ";

bool revicing_data = false;
String BUFFER_BITS = "";
String client_buffer = "";
char END_TOKEN = '\n';
char SEP_TOKEN = ',';

  EthernetClient client;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  DUCOID = get_DUCOID();
  //set connection 
  Ethernet.begin(mac);

  #ifdef __DEBUG__
  Serial.begin(9600);
  Serial.println("starting miner...");
  Serial.print(DUCOID);
  #endif

  //microfix
  
  resolvePool();

  
}

void loop() {

  if(client.connect(pool,port)){
  client.print(VER);
  waitForClientData();

  #ifdef __DEBUG__
  String server_version = getValue(client_buffer, SEP_TOKEN, 1);
  Serial.println(server_version);
  #endif
  while (client.connected()){
    memset(job, 0, job_maxsize);
    #if defined(ARDUINO_ARCH_AVR)
        PORTB = PORTB & B11011111;
    #else
        digitalWrite(LED_BUILTIN, HIGH);
    #endif
    
    JOB_REQUEST();
    waitForClientData();
    String last_block_hash = getValue(client_buffer, SEP_TOKEN, 0);
    String expected_hash = getValue(client_buffer, SEP_TOKEN, 1);
    unsigned int difficulty = getValue(client_buffer, SEP_TOKEN, 2).toInt();
    #ifdef __DEBUG__
    Serial.println("JOB_RECIVED RAW: "+ client_buffer);
    Serial.println("Job received: "
                 + last_block_hash
                 + " "
                 + expected_hash
                 + " "
                 + String(difficulty));
    #endif
    //the start time
    uint32_t startTime = micros();

    #ifdef __DEBUG__
    //DEBUG
    Serial.println("starting hashing");
    //----
    #endif
    
    //DEBUG
    #ifdef __DEBUG__
    Serial.println("end hashing");
    #endif
    
    ducos1result = ducos1a(last_block_hash.c_str(), expected_hash.c_str(), difficulty);
    uint32_t elapsedTime = micros() - startTime;
    last_block_hash = "";
    expected_hash = "";
    difficulty = 10;

    float elapsed_time_s = elapsedTime / 1000000.0f;
    float hashrate = ducos1result / elapsed_time_s;

    #ifdef __DEBUG__
    Serial.print("hashrate: ");
    Serial.print(hashrate);
    Serial.print(" speed: ");
    Serial.println(elapsed_time_s);
    #endif

    client.print(String(ducos1result)
                   + ","
                   + String(hashrate)
                   + "," + String(miner_version)
                   + ","
                   + String(RIG_IDENTIFIER)
                   + ","
                   + String(DUCOID));

    

    waitForClientData();

    #ifdef __DEBUG__
    Serial.println(client_buffer);
    #endif
    
    #if defined(ARDUINO_ARCH_AVR)
        PORTB = PORTB | B00100000;
    #else
        digitalWrite(LED_BUILTIN, LOW);
    #endif
    delay(90);
    }
    }
}

void testPing(){
    if(client.connect(pool,port)){
      #ifdef __DEBUG__
      Serial.println("testing pool conection");
      #endif
      client.println("MOTD");
    } else {
      #ifdef __DEBUG__
      Serial.println("error: ");
      #endif
      delay(1);
  }
}

void JOB_REQUEST() {
      String petition = "JOB"
                        + SEPARATOR + Username
                        + SEPARATOR + "AVR"
                        + SEPARATOR + key;
      client.print(petition);
      #ifdef __DEBUG__
      
      #endif   
}

String get_DUCOID() {
  String ID = "DUCOID";
  char buff[4];
  for (size_t i = 0; i < 8; i++) {
    sprintf(buff, "%02X", (uint8_t)UniqueID8[i]);
    ID += buff;
  }
  return ID;
}

void lowercase_hex_to_bytes(char const * hexDigest, uint8_t * rawDigest) {
  for (uint8_t i = 0, j = 0; j < SHA1_HASH_LEN; i += 2, j += 1) {
    uint8_t x = hexDigest[i];
    uint8_t b = x >> 6;
    uint8_t r = ((x & 0xf) | (b << 3)) + b;

    x = hexDigest[i + 1];
    b = x >> 6;

    rawDigest[j] = (r << 4) | (((x & 0xf) | (b << 3)) + b);
  }
}

// DUCO-S1A hasher
uintDiff ducos1a(char const * prevBlockHash, char const * targetBlockHash, uintDiff difficulty) {
  #if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
    // If the difficulty is too high for AVR architecture then return 0
    if (difficulty > 655) return 0;
  #endif

  uint8_t target[SHA1_HASH_LEN];
  lowercase_hex_to_bytes(targetBlockHash, target);

  uintDiff const maxNonce = difficulty * 100 + 1;
  return ducos1a_mine(prevBlockHash, target, maxNonce);
}

uintDiff ducos1a_mine(char const * prevBlockHash, uint8_t const * target, uintDiff maxNonce) {
  static duco_hash_state_t hash;
  duco_hash_init(&hash, prevBlockHash);

  char nonceStr[10 + 1];
  for (uintDiff nonce = 0; nonce < maxNonce; nonce++) {
    ultoa(nonce, nonceStr, 10);

    uint8_t const * hash_bytes = duco_hash_try_nonce(&hash, nonceStr);
    if (memcmp(hash_bytes, target, SHA1_HASH_LEN) == 0) {
      return nonce;
    }
  }

  return 0;
}

//sorry for copy function but i need the function 
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int max_index = data.length() - 1;

  for (int i = 0; i <= max_index && found <= index; i++) {
    if (data.charAt(i) == separator || i == max_index) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == max_index) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void waitForClientData(void) {
  client_buffer = "";
  while (client.connected()) {
    if (client.available()) {
      client_buffer = client.readStringUntil(END_TOKEN);
      if (client_buffer.length() == 1 && client_buffer[0] == END_TOKEN)
        client_buffer = "???\n"; // NOTE: Should never happen
      break;
    }
  }

  #ifdef __DEBUG__
  Serial.println("client buffer: "+client_buffer);
  #endif
}


void resolvePool() {
  // BLOQUE 1: Si se define MANUAL_POOL, solo compilamos esto y ahorramos todo el código HTTP
  #ifdef __MANUAL_POOL
    pool[0] = 192;
    pool[1] = 168;
    pool[2] = 0;
    pool[3] = 253;
    port = 8080;
    
    #ifdef __DEBUG__
      Serial.println(F("Manual Pool Selected")); // F() ahorra RAM
    #endif

  // BLOQUE 2: Si NO es manual, compilamos la lógica de red
  #else 
    const char* server = "server.duinocoin.com";

    if (client.connect(server, 80)) {
      // Usamos F() para guardar strings en Flash y no ocupar RAM al ejecutarse
      client.print(F("GET /getPool HTTP/1.1\r\n"
                   "Host: server.duinocoin.com\r\n"
                   "Connection: close\r\n\r\n"));

      // client.find busca una cadena en el stream y descarta todo lo anterior.
      // Buscamos el final de los headers HTTP (\r\n\r\n)
      if (client.find("\r\n\r\n")) {
        
        // Buscamos la clave "ip":"
        if (client.find("\"ip\":\"")) {
          // parseInt lee caracteres numéricos hasta encontrar un no-numérico (el punto o la comilla)
          // Esto parsea "192.168.0.1" automáticamente sin usar String ni buffers
          pool[0] = client.parseInt();
          pool[1] = client.parseInt();
          pool[2] = client.parseInt();
          pool[3] = client.parseInt();
        }

        // Buscamos la clave "port":
        if (client.find("\"port\":")) {
          port = client.parseInt();
        }
      }
      
      client.stop();

      #ifdef __DEBUG__
        Serial.print(F("Pool: "));
        Serial.print(pool[0]); Serial.print('.');
        Serial.print(pool[1]); Serial.print('.');
        Serial.print(pool[2]); Serial.print('.');
        Serial.print(pool[3]);
        Serial.print(F(" Port: ")); Serial.println(port);
      #endif

    } else {
      #ifdef __DEBUG__
        Serial.println(F("Connection failed"));
      #endif
    }
  #endif
}