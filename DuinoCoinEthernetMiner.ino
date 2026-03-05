/*
HELLO GUYS THIS CODE is auto miner, made with pumafron afk, the code mine only using 100% arduino and ethernet shield
thannks you LDarki for help me to fix connection error
the proyect is arduino miner to dunicoin made with revox
thanks you Joybed to fix the hashrate problem
*/

//#define __DEBUG__ //enables or disables serial console, disabling it may result in higher hashrates, uncomment if you want serial enabled
//#define __MANUAL_POOL


#ifdef __DEBUG__
  #define DEBUG_BEGIN(baud) Serial.begin(baud)
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_BEGIN(baud) 
  #define DEBUG_PRINT(x)    
  #define DEBUG_PRINTLN(x)  
#endif

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
const char * miner_version = "Pumitahou 3.1";
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

  DEBUG_BEGIN(9600);
  DEBUG_PRINTLN("starting miner...");
  DEBUG_PRINT(DUCOID);

  //microfix
  resolvePool();
}

void loop() {

  if(client.connect(pool,port)){
    client.print(VER);
    waitForClientData();

    // Este bloque es necesario porque server_version solo existe en modo DEBUG
    #ifdef __DEBUG__
    String server_version = getValue(client_buffer, SEP_TOKEN, 1);
    DEBUG_PRINTLN(server_version);
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

      DEBUG_PRINTLN("JOB_RECIVED RAW: "+ client_buffer);
      DEBUG_PRINTLN("Job received: "
                   + last_block_hash
                   + " "
                   + expected_hash
                   + " "
                   + String(difficulty));

      //the start time
      uint32_t startTime = micros();

      DEBUG_PRINTLN("starting hashing");
      
      ducos1result = ducos1a(last_block_hash.c_str(), expected_hash.c_str(), difficulty);
      uint32_t elapsedTime = micros() - startTime;

      DEBUG_PRINTLN("end hashing");

      last_block_hash = "";
      expected_hash = "";
      difficulty = 10;

      float elapsed_time_s = elapsedTime / 1000000.0f;
      float hashrate = ducos1result / elapsed_time_s;

      DEBUG_PRINT("hashrate: ");
      DEBUG_PRINT(hashrate);
      DEBUG_PRINT(" speed: ");
      DEBUG_PRINTLN(elapsed_time_s);

      client.print(String(ducos1result)
                     + ","
                     + String(hashrate)
                     + "," + String(miner_version)
                     + ","
                     + String(RIG_IDENTIFIER)
                     + ","
                     + String(DUCOID));

      waitForClientData();

      DEBUG_PRINTLN(client_buffer);
      
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
    DEBUG_PRINTLN("testing pool conection");
    client.println("MOTD");
  } else {
    DEBUG_PRINTLN("error: ");
    delay(1);
  }
}

void JOB_REQUEST() {
  String petition = "JOB"
                    + SEPARATOR + Username
                    + SEPARATOR + "AVR"
                    + SEPARATOR + key;
  client.print(petition);
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

  DEBUG_PRINTLN("client buffer: "+client_buffer);
}

void resolvePool() {
  #ifdef __MANUAL_POOL
    pool[0] = 192;
    pool[1] = 168;
    pool[2] = 0;
    pool[3] = 253;
    port = 8080;
    DEBUG_PRINTLN("Manual Pool Selected");

  #else
    const char* server = "server.duinocoin.com";

    if (client.connect(server, 80)) {
      client.print("GET /getPool HTTP/1.0\r\nHost: server.duinocoin.com\r\n\r\n");

      if (client.find("\"ip\":\"")) {
        pool[0] = client.parseInt();
        pool[1] = client.parseInt();
        pool[2] = client.parseInt();
        pool[3] = client.parseInt();
      }
      if (client.find("\"port\":")) {
        port = client.parseInt();
      }

      client.stop();

      DEBUG_PRINT("Pool: ");
      DEBUG_PRINT(pool[0]); DEBUG_PRINT('.');
      DEBUG_PRINT(pool[1]); DEBUG_PRINT('.');
      DEBUG_PRINT(pool[2]); DEBUG_PRINT('.');
      DEBUG_PRINT(pool[3]);
      DEBUG_PRINT(" Port: "); DEBUG_PRINTLN(port);

    } else {
      DEBUG_PRINTLN("Connection failed");
    }
  #endif
}