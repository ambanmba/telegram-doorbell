#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LITTLEFS.h>
#include <time.h>    

const char* ssid      = "xxxxxxxxxxxxx";
const char* password  = "yyyyyyyyyyyyy";
const char* bot_token = "000000000:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

long timezone = 1; 
byte daysavetime = 1;

const unsigned long BOT_MTBS = 1000; // mean time between scan messages

unsigned long bot_lasttime;          // last time messages' scan has been done
WiFiClientSecure secured_client;
UniversalTelegramBot bot(bot_token, secured_client);
File myFile;

bool isMoreDataAvailable()
{
  return myFile.available();
}

byte getNextByte()
{
  return myFile.read();
}

void handleNewMessages(int numNewMessages)
{
  String chat_id = bot.messages[0].chat_id;
  String message = bot.messages[0].text;
  String file_name = "/snap.jpeg";                 // Local filename
  String url = "http://192.168.4.19/snap.jpeg"; // File to retrieve from camera

  Serial.print("[TELEGRAM] Message from user: ");
  Serial.println(chat_id);
  Serial.print("[TELEGRAM] Message text: ");
  Serial.println(message);
  Serial.println("[TELEGRAM] Sending confirmation message...");
  bot.sendMessage(bot.messages[0].chat_id, "Retrieving Image from camera, please wait....", "");
  
  downloadAndSaveFile(file_name, url);

  bot.sendMessage(bot.messages[0].chat_id, "Image retrieved. Sending to Telegram Server....", "");
  
  myFile = LITTLEFS.open(file_name, FILE_READ);

  if (myFile)
  {
    Serial.print("[TELEGRAM] Sending image: ");
    Serial.print(file_name);
    Serial.println("....");

    String sent = bot.sendPhotoByBinary(chat_id, "image/jpeg", myFile.size(),
                                        isMoreDataAvailable,
                                        getNextByte, nullptr, nullptr);

    if (sent)
    {
      Serial.println("[TELEGRAM] Image was successfully sent.");
    }
    else
    {
      Serial.println("[TELEGRAM] Image was not sent.");
    }
    myFile.close();
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("[TELEGRAM] Error opening photo");
  }
}

void setup(){
    Serial.begin(115200);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.print("[STARTUP] Connecting to Wifi SSID ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[STARTUP] WiFi connected");
    Serial.print("[STARTUP] IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("[STARTUP] Contacting Time Server");
	  configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
	  struct tm tmstruct ;
    delay(2000);
    tmstruct.tm_year = 0;
    getLocalTime(&tmstruct, 5000);
	  Serial.printf("[STARTUP] Time is: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct.tm_year)+1900,( tmstruct.tm_mon)+1, tmstruct.tm_mday,tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);
    Serial.println("[STARTUP] Mounting LittleFS");    
      if(!LITTLEFS.begin(true)){
        Serial.println("[STARTUP] LittleFS Mount Failed");
        return;
    }
    Serial.println("[STARTUP] LittleFS Mounted");    
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
}

void loop(){
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      Serial.println("[TELEGRAM] Received Message");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}

void downloadAndSaveFile(String fileName, String  url){
  HTTPClient http;
  Serial.print("[FILE] Local filename: ");
  Serial.println(fileName);

  Serial.println("[HTTP] begin...");
  Serial.print("[HTTP] Destination URL: ");
  Serial.println(url);
  http.begin(url);
 
  Serial.printf("[HTTP] GET...\n", url.c_str());
  // start connection and send HTTP header
  int httpCode = http.GET();
  if(httpCode > 0) {
      // HTTP header has been sent and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      Serial.println("[FILE] open file for writing...");
     
      File file = LITTLEFS.open(fileName, FILE_WRITE);
      //File file = LITTLEFS.open("/snap.jpeg", FILE_WRITE);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {

          // get length of document (is -1 when Server sends no Content-Length header)
          int len = http.getSize();
          Serial.printf("[FILE] size is: ");
          Serial.println(len);

          // create buffer for read
          uint8_t buff[128] = { 0 };

          // get tcp stream
          WiFiClient * stream = http.getStreamPtr();

          // read all data from server
          while(http.connected() && (len > 0 || len == -1)) {
              // get available data size
              size_t size = stream->available();
              if(size) {
                  // read up to 128 byte
                  int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                  file.write(buff, c);
                  if(len > 0) {
                      len -= c;
                  }
              }
              delay(1);
          }
          Serial.println("[HTTP] connection closed or file end.");
          Serial.println("[FILE] closing file");
          file.close();
      }
  }
  http.end();
}
