#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
}

String output4State = "off";
String output5State = "off";

// Assign output variables to GPIO pins
const int output4 = 4;
const int output5 = 5;

String header;
// SSID to connect to
static const char* ssid = "CAMPUS_SECURED";
// Username for authentification
static const char* username = "#######";
// Password for authentication
static const char* password = "#######";


WiFiServer server(80);
WebSocketsServer webSocket = WebSocketsServer(8080);

char buff[20];
String ip;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(output4, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output4, LOW);
  pinMode(output5, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, LOW);
  delay(500);

// WPA2 Connection starts here
// Setting ESP into STATION mode only (no AP mode or dual mode)
  wifi_set_opmode(STATION_MODE);
  struct station_config wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));
  strcpy((char*)wifi_config.ssid, ssid);
  wifi_station_set_config(&wifi_config);
  wifi_station_clear_cert_key();
  wifi_station_clear_enterprise_ca_cert();
  wifi_station_set_wpa2_enterprise_auth(1);
  wifi_station_set_enterprise_identity((uint8*)username, strlen(username));
  wifi_station_set_enterprise_username((uint8*)username, strlen(username));
  wifi_station_set_enterprise_password((uint8*)password, strlen(password));
  wifi_station_connect();

  // Wait for connection AND IP address from DHCP
  Serial.println();
  Serial.println("Waiting for connection and IP Address from DHCP");
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  IPAddress myAddr = WiFi.localIP();
  sprintf(buff, "%d.%d.%d.%d", myAddr[0], myAddr[1], myAddr[2], myAddr[3]);
  ip = String(buff);
  Serial.println(ip);
  Serial.write("\r\nStart Webserver");
  server.begin();
  Serial.write("\r\nStart Websocket");
  webSocket.begin();
 
}

void loop() {
  webSocket.loop();
  WiFiClient client = server.available();

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("GPIO 5 on");
              output5State = "on";
              digitalWrite(output5, HIGH);
            } else if (header.indexOf("GET /5/off") >= 0) {
              Serial.println("GPIO 5 off");
              output5State = "off";
              digitalWrite(output5, LOW);
            } else if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("GPIO 4 on");
              output4State = "on";
              digitalWrite(output4, HIGH);
            } else if (header.indexOf("GET /4/off") >= 0) {
              Serial.println("GPIO 4 off");
              output4State = "off";
              digitalWrite(output4, LOW);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP8266 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 5  
            client.println("<p>GPIO 5 - State " + output5State + "</p>");
            // If the output5State is off, it displays the ON button       
            if (output5State=="off") {
              client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 4  
            client.println("<p>GPIO 4 - State " + output4State + "</p>");
            // If the output4State is off, it displays the ON button       
            if (output4State=="off") {
              client.println("<p><a href=\"/4/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/4/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
