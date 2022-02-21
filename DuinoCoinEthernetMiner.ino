/*
HELLO GUYS THIS CODE is auto miner, made with pumafron afk, the code mine only using 100% arduino and ethernet shield
thannks you LDarki for help me to fix connection error
the proyect is arduino miner to dunicoin made with revox
thanks you Joybed to fix the hashrate problem
*/

//#define __DEBUG__ //enables or disables serial console, disabling it may result in higher hashrates, uncomment if you want serial enabled

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
#include "sha1.h"
#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ip[] = {192,168,1,177};

// the pool ip final 
byte pool[] = {162,55,103,174};
unsigned short port = 6000;

//miner global variables
String Username = "Puma"; //put your username here
const char* RIG_IDENTIFIER = "AVR ethernet"; //put your rig identifier here

String lastblockhash = "";
String newblockhash = "";
String DUCOID = "";
uintDiff difficulty = 0;
uintDiff ducos1result = 0;
const uint16_t job_maxsize = 104;  
uint8_t job[job_maxsize];
Sha1Wrapper Sha1_base;

//client variables SETTINGS
const char * miner_version = "PumaFron miner 3.0";
String VER = "3.0";
unsigned short SOC_TIMEOUT = 15;
unsigned short REPORT_TIME = 120;
unsigned short AVR_TIMEPUT = 7;
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
  Ethernet.begin(mac,ip);
  #ifdef __DEBUG__
  Serial.begin(9600);
  Serial.println("starting miner...");
  Serial.print(DUCOID);
  #endif
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
    ducos1result = ducos1a(last_block_hash, expected_hash, difficulty);
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

//this function is to test pool ping command pls remove to final compilation
//the function get motd to pool
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
      delay(1)
  }
}

void JOB_REQUEST() {
      String petition = "JOB," + String(Username) + "," + String("AVR");
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

// DUCO-S1A hasher
uintDiff ducos1a(String lastblockhash, String newblockhash, uintDiff difficulty) {
  newblockhash.toUpperCase();
  const char *c = newblockhash.c_str();
  uint8_t final_len = newblockhash.length() >> 1;
  for (uint8_t i = 0, j = 0; j < final_len; i += 2, j++)
    job[j] = ((((c[i] & 0x1F) + 9) % 25) << 4) + ((c[i + 1] & 0x1F) + 9) % 25;

    // Difficulty loop
  #if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
    // If the difficulty is too high for AVR architecture then return 0
    if (difficulty > 655) return 0;
  #endif
  Sha1_base.init();
  Sha1_base.print(lastblockhash);
  for (uintDiff ducos1res = 0; ducos1res < difficulty * 100 + 1; ducos1res++) {
    //Sha1.init();
    //Sha1.print(lastblockhash + String(ducos1res));
    Sha1 = Sha1_base;
    Sha1.print(String(ducos1res));
    // Get SHA1 result
    uint8_t *hash_bytes = Sha1.result();
    if (memcmp(hash_bytes, job, SHA1_HASH_LEN * sizeof(char)) == 0) {
      // If expected hash is equal to the found hash, return the result
      return ducos1res;
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
