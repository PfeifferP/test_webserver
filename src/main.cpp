#include <Arduino.h>

#ifdef ESP32
#include <FS.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#endif
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

// SKETCH BEGIN
AsyncWebServer server(80);


const char* ssid = "*******";
const char* password = "*******";
const char * hostName = "esp-async";
const char* http_username = "admin";
const char* http_password = "admin";
//
const int buf_size = 32; 
char s[buf_size];

void printDirectory(File dir,int numTabs = 3);


//
void setup(){
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(hostName);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }

  

  MDNS.addService("http","tcp",80);

  Serial.print(F("Initializing FS..."));
    if (LittleFS.begin()){
        Serial.println(F("done."));
    }else{
        Serial.println(F("fail."));
    }

  

#ifdef ESP32
  server.addHandler(new SPIFFSEditor(LittleFS, http_username,http_password));
#elif defined(ESP8266)
  server.addHandler(new SPIFFSEditor(http_username,http_password));
#endif
  
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.htm");

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char*)data);
    if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });
  server.begin();

  // To format all space in LittleFS
    // LittleFS.format()
 
    // Get all information of your LittleFS
 
    unsigned int totalBytes = LittleFS.totalBytes();
    unsigned int usedBytes = LittleFS.usedBytes();
    unsigned int freeBytes;
 
    Serial.println("File system info:");

    snprintf(s, buf_size, "\tTotal: %10d bytes", totalBytes);
    puts(s);
 
    snprintf(s, buf_size, "\tUsed: %11d bytes", usedBytes);
    puts(s);

    freeBytes = totalBytes - usedBytes;
    snprintf(s, buf_size, "\tFree: %11d bytes", freeBytes);
    puts(s);
 
    Serial.println();
 
    // Open dir folder
    File dir = LittleFS.open("/");
    // Cycle all the content
    printDirectory(dir);
}

void loop(){
  
}

void printDirectory(File dir, int numTabs) {
    while (true) {
        File entry = dir.openNextFile();
        if (! entry) {
            // no more files
            break;
        }
        for (uint8_t i = 0; i < numTabs; i++) {
          Serial.print('\t');
        }
        Serial.print(entry.name());
        if (entry.isDirectory()) {
          Serial.println("/");
          printDirectory(entry, numTabs + 1);
        } else {
          // files have sizes, directories do not
          Serial.print("\t\t");
          Serial.println(entry.size(), DEC);
        }
        entry.close();
    }
}