/*
 * Lucas Berbesson for La Fabrique DIY
 *
 *
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>

#include "play.h"


int knockReading = 0;      // variable to store the value read from the sensor pin
const int knockSensor = A0; // the piezo is connected to analog pin 0
const int threshold = 15;  // threshold value to decide when the detected sound is a knock or not
int score = 0;
unsigned long last_bounce = 0;
boolean playing = false;

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Ready!");
        }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

            if(payload[0] == '#') {
                // we get RGB data

                // decode rgb data
                uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);

            }

            break;
    }

}

void setup() {
    USE_SERIAL.begin(115200);
    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFiMulti.addAP("Wifi name", "Wifi password");

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    // start webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    if(MDNS.begin("esp8266")) {
        USE_SERIAL.println("MDNS responder started");
        USE_SERIAL.println(WiFi.localIP());
    }

    // handle index
    server.on("/", []() {
        // send index.html
        String s = MAIN_page;
        server.send(200, "text/html", s);
    });
    server.begin();
    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
}

void loop() {
    webSocket.loop();
    server.handleClient();
    knockReading = analogRead(A0);
    delay(1);
    unsigned long currentMillis = millis();
    // No bounce during more than 2 seconds is GAME OVER
    if (currentMillis - last_bounce > 2000 && playing == true) {
      webSocket.sendTXT(0,"Game over");
      score = -1;
      playing=false;
      delay(50);
    }
    // If there is a bounce, send a message to websocket
    if (knockReading >= threshold) {
      score = score + 1;
      playing = true;
      last_bounce = millis();
      String thisString = String(score);
      webSocket.sendTXT(0,thisString);
      delay(50);
    } 
}

