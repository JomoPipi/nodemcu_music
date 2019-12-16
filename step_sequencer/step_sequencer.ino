#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

File fsUploadFile;                                    // a File variable to temporarily store the received file

const char *ssid = "ESP8266 Access Point"; // The name of the Wi-Fi network that will be created
const char *password = "thereisnospoon";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";

const char* mdnsName = "esp8266"; // Domain name for the mDNS responder





struct player_t {
  unsigned long T;
  const int pin = D5;
  bool muted = false;
  long start_ms = millis();
  long ms = 0;
  int currentNote=0;
  int delayMS = 100; // (tempo) - 120bpm
} player;

typedef struct {
  bool active = true;
  int pitch = 400;
  int waveType = 0;
  float mod_period = 1;
  float mod_intensity = 0.05;
} Note;

Note notes[8];

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  pinMode(player.pin, OUTPUT);

  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

//   startOTA();                  // Start the OTA service

  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server

  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler
  Serial.println(atoi("hello"));
  for(int i = 0; i < 8; i++) {
    notes[i].pitch += i*114;
    if (i > 5)
      notes[i].active = 0;
  }
}

/*__________________________________________________________LOOP__________________________________________________________*/


// void delayMicros(long mics) {
//   // to be benchmarked:
//   if (mics >> 14 > 0)
// //  if (mics > 16000)
//     delay(mics / 1000);
//   else
//     delayMicroseconds(mics);
// }

const double pi2 = PI/2.0;
double waveFunction(int f, double x) {
  switch (f) {
    case 0: return sin(x);                // sine
    case 1: return asin(sin(x)); // tri
    case 2: return sin(x) > 0 ? 1 : -1; // square
    case 3: return atan(tan(x));          // saw
    case 4: return -atan(tan(x));         // reverse saw
    case 5: return max(tan(-x/PI+pi2),tan(-x/PI));       // X
    case 6: return max(tan(x/PI+pi2),tan(x/PI));         // Y
    case 7: return cos(x)*3+sin(x*10);     // noisy
    case 8: return cos(x)*3+sin(x*50);     // noisyer
    case 9: return atan(tan(x))-tan(cos(x)); // Z
  }
}

void loop() {
  webSocket.loop();                           // constantly check for websocket events
    int x = 0;
  if (player.ms >= player.start_ms + player.delayMS) {
    do {
      player.currentNote = (player.currentNote+1) % 8;
    } while (x++ < 8 && !notes[player.currentNote].active);
    player.start_ms = player.ms;
    player.T=0; // restart the mod wave;
  }
  player.ms = millis();
  const Note note = notes[player.currentNote];
  const double wf = waveFunction(note.waveType, ++player.T * note.mod_period / 1000.0 );
  if (x < 8) // mute by active step
    tone(player.pin, note.pitch + wf * note.mod_intensity);
  else
    noTone(player.pin);

  server.handleClient();                      // run the server
//  ArduinoOTA.handle();                        // listen for OTA events
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received

      Serial.printf("[%u] get Text: %s\n", num, payload);

      //  note.pitch = atoi((const char *)payload);

      // typedef struct {
      //   int pitch=220;
      //   int waveType=0;
      //   float mod_intensity=0.05;
      //   float mod_period=1;
      // } Note;

      // Note notes[8];

      // struct player_t {
      //   unsigned long T;
      //   const int pin = D5;
      //   int currentNote=0;
      //   int start_ms;
      //   int tempo;
      //   int ms;
      // } player;
      
      // const payload = [pitch,m_amount,m_period].join`:`
      // [tempo, 8 notes (each with 5 properties)][
        //     active,
        //     pitch,
        //     modA,
        //     modP,
        //     wave
        // ]
      const int datalength = 1 + 8 * 5;
      int data[datalength];
      for(int i = 0; i < datalength; i++) data[i] = 0;
      for(int i = 0, j = 0; i < lenght; ++i) {
        if (payload[i]=='C') return;
        char c = payload[i] - 48;
        if (c == 10) {
          ++j;
          continue;
        }
        data[j] = data[j] * 10 + c;
      }
      Serial.print("BPM =");
      Serial.println(data[0]);
      const int myDelay = 1000*(30.0/data[0]); // 8th note steps
      player.delayMS = myDelay;
      Serial.print("delay =");
      Serial.println(player.delayMS);
      for (int i = 1; i < datalength; i++) {
        const int noteIdx = (i-1)/5;
        const int x = (i-1) % 5;
        const int y = data[i];
        if (x == 0)
          notes[noteIdx].active = y;
        else if (x == 1)
          notes[noteIdx].pitch = y;
        else if (x == 2)
          notes[noteIdx].mod_intensity = y / 100.0;
        else if (x == 3)
          notes[noteIdx].mod_period = y / 100.0;
        else if (x == 4)
          notes[noteIdx].waveType = y;
      }
      // Serial.print("player.pitch = ");
      // Serial.print(player.pitch);
      // Serial.print(", ");
      // Serial.print("player.mod_intensity = ");
      // Serial.print(player.mod_intensity);
      // Serial.print(", ");
      // Serial.print("player.mod_period = ");
      // Serial.println(player.mod_period);
      break;
  }
}



/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

//  WiFi.softAP(ssid, password);             // Start the access point 
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");

  wifiMulti.addAP("Fernando", "7862529636");
  wifiMulti.addAP("LAMC", "");
  wifiMulti.addAP("iPhone", "12341234");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if (WiFi.softAPgetStationNum() == 0) {     // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    int times = 0;
    while (dir.next()) {                      // List the file system contents
      times++;
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
    Serial.print("number of contents: ");
    Serial.println(times);
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found. Damn");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                        // If there's a compressed version available
      path += ".gz";                                      // Use the compressed verion
    File file = SPIFFS.open(path, "r");                   // Open the file
    size_t sent = server.streamFile(file, contentType);   // Send it to the client
    file.close();                                         // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
