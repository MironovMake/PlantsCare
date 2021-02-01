// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
// Replace with your network credentials
const char* ssid     = "wifi name";
const char* password = "passowrd";
// some time properties
const long utcOffsetInSeconds = 3600;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String ledPinState = "off";

// Assign output variables to GPIO pins
const int ledPin = D6;
int bright = 900;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//some my variables 
int per; // percent of brightness
int var_clock; 
bool flag = 0;
int previous_state=bright;
void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(ledPin, OUTPUT);
  // Set outputs to LOW

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  timeClient.begin();
}

void loop() {
  // read time from router
  timeClient.update();
  var_clock = timeClient.getHours() + 6; // my router give me time less than my by 6
  // turn on led for plants at 14:00
  if (var_clock > 14 && var_clock < 23 && flag == 0) {
    analogWrite(ledPin, bright);
    Serial.println(bright);
    ledPinState = "on";
    flag = 1;
  } // off led at 23:00
  else if ((var_clock < 14 || var_clock > 23) && flag == 1) {
    analogWrite(ledPin, LOW);
    ledPinState = "off";
    Serial.println(bright);
    flag == 0;
  }


  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
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

            // turns the buttons
            if (header.indexOf("GET /6/on") >= 0) {
              ledPinState = "on";
              for (int dutyCycle = 0; dutyCycle < bright; dutyCycle++) {
                analogWrite(ledPin, dutyCycle);
                delay(1);
              }
              Serial.println(bright);
            } else if (header.indexOf("GET /6/off") >= 0) {
              Serial.println(bright);
              ledPinState = "off";
              digitalWrite(ledPin, LOW);
              // if button "brighter" touch
            } else if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("brighter");
              bright += 100;
              ledPinState = "on";
              if (bright > 1023) {
                bright = 1023;
              }
              Serial.println(bright);
              analogWrite(ledPin, bright);
              // if button "darkner" touch
            } else if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("darkner");
              bright -= 100;
              ledPinState = "on";
              if (bright < 0) {
                bright = 0;
                ledPinState = "off";
              }
              analogWrite(ledPin, bright);
              Serial.println(bright);
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
            client.println("<body><h1>State " + ledPinState + "</h1>");

            // Display current state, and ON/OFF buttons for GPIO 6
            client.println("<p> Mironov's Web Server</p>");
            // If the ledPinState is off, it displays the ON button
            if (ledPinState == "off") {
              client.println("<p><a href=\"/6/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/6/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            
            per = map(bright, 0, 1023, 0, 100);
            // Display current state, and ON/OFF buttons for brighter
            client.println("<p>" + String(per) + "</p>");
            // If the output4State is off, it displays the ON button
            client.println("<p><a href=\"/4/on\"><button class=\"button\">brighter</button></a></p>");

            // Display current state, and ON/OFF buttons for brighter
            client.println("<p>" + String(per) + "</p>");
            // If the output4State is off, it displays the ON button
            client.println("<p><a href=\"/5/on\"><button class=\"button\">darkner</button></a></p>");
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
