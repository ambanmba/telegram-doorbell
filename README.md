# telegram-doorbell
Very basic POC/MVP to capture an image from a Ubiquiti Doorbell (or any other Ubiquiti camera, or any other .jpg file that can be reached via http) and send it as a message in a Telegram bot.

**Prerequisites:** 
1. Make sure the .jpg is reachable from the same network. On a Ubiquiti camera see here: https://community.ui.com/questions/URL-to-take-a-camera-image/fe0c7645-97d9-44d5-a0a1-cabc5a14ebff
2. Get a Telegram Bot token: https://core.telegram.org/bots#botfather
3. Install the Universal Arduino Telegram Bot library: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot 

**Then modify the code as follows:**
1. Fill in your details on lines 9-11 (Wifi Credentials and Telegram Bot token)
2. Fill in the full path to the image snapshot on your camera on line 39

Then upload the sketch to your board and open the Serial Terminal to see what's happening

Then connect to the Telegram Bot and send /start
Then each subsequent message you send to the bot will trigger it to pull an image and send it

**Important Notes:**
I got this to work on an ESP8266 board. You need to keep in mind that the memory available is quite small so the .jpg file cannot be too large. Also keep in mind that it runs VERY slowly. It takes a bit over 10 seconds to pull a ~256k file from the camera and then it takes nearly 5 minutes to push it to Telegram.

Here is an example with time stamps (so you can see how slow it is) of all the steps as they are output to the Serial Terminal:

```
19:45:06.009 -> [TELEGRAM] Received Message
19:45:06.009 -> [TELEGRAM] Message from user: **redacted**
19:45:06.009 -> [TELEGRAM] Message text: /start
19:45:06.009 -> [TELEGRAM] Sending confirmation message...
19:45:06.009 -> [FILE] Local filename: snap.jpeg
19:45:06.009 -> [HTTP] begin...
19:45:06.009 -> [HTTP] Destination URL: **redacted**
19:45:06.009 -> [HTTP] GET...
19:45:06.009 -> [HTTP] GET... code: 200
19:45:06.009 -> [FILE] open file for writing...[FILE] size is: 256657
19:45:17.008 -> [HTTP] connection closed or file end.
19:45:17.008 -> [FILE] closing file
19:45:23.208 -> [TELEGRAM] Sending image: snap.jpeg....
19:49:17.818 -> [TELEGRAM] Image was successfully sent.
```
