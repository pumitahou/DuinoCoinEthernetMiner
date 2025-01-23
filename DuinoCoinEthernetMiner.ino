/*
HELLO GUYS THIS CODE is auto miner, made with pumafron afk, the code mine only using 100% arduino and ethernet shield
thannks you LDarki for help me to fix connection error
the proyect is arduino miner to dunicoin made with revox
thanks you Joybed to fix the hashrate problem
*/

#define __DEBUG__ //enables or disables serial console, disabling it may result in higher hashrates, uncomment if you want serial enabled

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

String lastblockhash = "";
String newblockhash = "";

String DUCOID = "";

uintDiff difficulty = 0;
uintDiff ducos1result = 0;

const uint16_t job_maxsize = 104;
uint8_t job[job_maxsize];

//client variables SETTINGS
const char * miner_version = "PumaFron miner 3.0";
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
  resolvePool();

  waitForClientData();
  String server_version = getValue(client_buffer, SEP_TOKEN, 1);

  #ifdef __DEBUG__
  Serial.println(server_version);
  #endif
}

void loop() {
  if(client.connect(pool,port)){
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
    
    Serial.println("Job received: "
                 + last_block_hash
                 + " "
                 + expected_hash
                 + " "
                 + String(difficulty));
    #endif
    //the start time
    uint32_t startTime = micros();
    //DEBUG
    Serial.println("starting hashing");
    //----
    ducos1result = ducos1a(last_block_hash.c_str(), expected_hash.c_str(), difficulty);
    last_block_hash = "";
    expected_hash = "";
    difficulty = 0;
    //DEBUG
    #ifdef __DEBUG__
    Serial.println("end hashing");
    #endif
    uint32_t elapsedTime = micros() - startTime;
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
      Serial.println(petition);
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
}


void resolvePool() {
  const char* server = "server.duinocoin.com";
  
  if (client.connect(server, 80)) {
    client.print("GET /getPool HTTP/1.1\r\n"
                 "Host: server.duinocoin.com\r\n"
                 "Connection: close\r\n\r\n");

    while (client.connected() && !client.available()) delay(1);

    // Skip HTTP headers
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break; // End of headers
    }

    // Read JSON body
    String jsonResponse = client.readString();
    client.stop();

    // Manual JSON parsing
    int ipStart = jsonResponse.indexOf("\"ip\":\"") + 6;
    int ipEnd = jsonResponse.indexOf('"', ipStart);
    String ipStr = jsonResponse.substring(ipStart, ipEnd);

    int portStart = jsonResponse.indexOf("\"port\":") + 7;
    int portEnd = jsonResponse.indexOf(',', portStart);
    port = jsonResponse.substring(portStart, portEnd).toInt();

    // Parse IP without sscanf
    int dot1 = ipStr.indexOf('.');
    int dot2 = ipStr.indexOf('.', dot1 + 1);
    int dot3 = ipStr.indexOf('.', dot2 + 1);
    
    pool[0] = ipStr.substring(0, dot1).toInt();
    pool[1] = ipStr.substring(dot1 + 1, dot2).toInt();
    pool[2] = ipStr.substring(dot2 + 1, dot3).toInt();
    pool[3] = ipStr.substring(dot3 + 1).toInt();

    #ifdef __DEBUG__
    Serial.print("Pool: ");
    Serial.print(pool[0]); Serial.print(".");
    Serial.print(pool[1]); Serial.print(".");
    Serial.print(pool[2]); Serial.print(".");
    Serial.print(pool[3]);
    Serial.print(" Port: "); Serial.println(port);
    #endif
  } else {
    #ifdef __DEBUG__
    Serial.println("Connection failed");
    #endif
  }
}