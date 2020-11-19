#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

#define WIFI_SSID "xxxxxxx"                                           //Enter your Wifi SSID Here
#define WIFI_PASSWORD "yyyyyyyyyy"                                    //Enter your Wifi Password Here
#define BOT_TOKEN "0000000000:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"    //Enter your Telegram BOT token here

const unsigned long BOT_MTBS = 1000; // mean time between scan messages

unsigned long bot_lasttime;          // last time messages' scan has been done
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

File myFile;
bool isMoreDataAvailable();
byte getNextByte();

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
  String file_name = "snap.jpeg";                                     // Local filename - can be anything
  String url = "http://192.168.1.111/snap.jpeg";                      // ENTER YOUR CAMERA SNAPSHOT LINK HERE

  Serial.print("[TELEGRAM] Message from user: ");
  Serial.println(chat_id);
  Serial.print("[TELEGRAM] Message text: ");
  Serial.println(message);
  
  downloadAndSaveFile(file_name, url);
  
  myFile = LittleFS.open(file_name, "r");

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

void setup()
{
  Serial.begin(115200);
  Serial.println();
  delay(1000); // Wait a sec for the serial port to come up
  Serial.println();
  
  Serial.print("[STARTUP] Mounting LittleFS disk....");
  if (!LittleFS.begin())
  {
    Serial.println("failed!");
    return;
  }
  Serial.println("done.");

  Serial.print("[STARTUP] Formatting LittleFS disk....");
  if (!LittleFS.format())
  {
    Serial.println("failed!");
    return;
  }
  Serial.println("done.");

  // attempt to connect to Wifi network:
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  Serial.print("[STARTUP] Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\n[STARTUP] WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  // Check NTP/Time, usually it is instantaneous and you can delete the code below.
  Serial.print("[STARTUP] Retrieving time: ");
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(1000);
    now = time(nullptr);
  }
  Serial.println(now);
  Serial.println("[STARTUP] Initialisation complete, listening for messages...");
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
      Serial.print("[FILE] open file for writing...");
     
      File file = LittleFS.open(fileName, "w+");

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
                  // write it to Serial
                  //Serial.write(buff, c);
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


void loop()
{
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
