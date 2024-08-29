/*******************************************************************
    A sample project for making a HTTP/HTTPS GET request on an ESP8266

    It will connect to the given request and print the body to
    serial monitor

    Parts:
    D1 Mini ESP8266 * - http://s.click.aliexpress.com/e/uzFUnIe

 *  * = Affilate

    If you find what I do usefuland would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// ----------------------------
// Standard Libraries
// ----------------------------

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

//------- Replace the following! ------
char ssid[] = "Kroneinc";         // your network SSID (name)
char password[] = "5197789927"; // your network key

// For Non-HTTPS requests
// WiFiClient client;

// For HTTPS requests
WiFiClientSecure client;

// Just the base of the URL you want to connect to
#define TEST_HOST "github.com"
// #define TEST_HOST "api.coingecko.com"

// OPTIONAL - The fingerprint of the site you want to connect to.
#define TEST_HOST_FINGERPRINT "E7 03 5B CC 1C 18 77 1F 79 2F 90 86 6B 6C 1D F8 DF AA BD C0"
// The finger print will change every few months.

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // If you don't need to check the fingerprint
  client.setInsecure();

  makeHTTPRequest();
}

void makeHTTPRequest() {

  String response = "";
  
  // Open a connection to the server
  if (!client.connect(TEST_HOST, 443)) {
    Serial.println("Connection failed");
    return;
  }
  
  Serial.println("Connected to server");

  yield();

  // Send HTTP request
  // This is the second half of a request (everything that comes after the base URL)
  // HTTP 1.0 is ideal as the response wont be chunked
  // But some API will return 1.1 regardless, so we need
  // to handle both.
  client.print("GET /users/Exploser/contributions?from=2024-07-31&to=2024-08-27 HTTP/1.1\r\n");

  //Headers
  client.print("Host: ");
  client.println(TEST_HOST);
  // client.println("Connection: close");
  client.println(F("Cache-Control: no-cache"));
  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    // return;
  }

  Serial.println("Fetch Done");

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));

  // Check if it responded "OK" with either HTTP 1.0 or 1.1
  if (strcmp(status, "HTTP/1.0 200 OK") != 0 || strcmp(status, "HTTP/1.1 200 OK") != 0)
  {
    {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
      // return;
    }
  }

  Serial.print(F("GOING INTO THE DEEP"));
  
  while (client.available() && client.peek() != '{' && client.peek() != '[')
    {
      char c = 0;
      client.readBytes(&c, 1);
      Serial.print(c);
      Serial.println("BAD");
    }

  // Print the response body
  while (client.available()) {
    char c = client.read();
    // Serial.print(c);
    response += c;
  }

  extractTDAttributes(response);
}

void extractTDAttributes(String html) {
  int startIndex = 0;
  while ((startIndex = html.indexOf("<td", startIndex)) != -1) {
    int endIndex = html.indexOf(">", startIndex);
    if (endIndex == -1) break;

    String tdElement = html.substring(startIndex, endIndex + 1);

    String id = extractAttribute(tdElement, "id");
    String dataLevel = extractAttribute(tdElement, "data-level");

    if (id.length() > 0 && dataLevel.length() > 0) {
      Serial.println("id: " + id + ", data-level: " + dataLevel);
    }

    startIndex = endIndex + 1;
  }
}

String extractAttribute(String element, String attribute) {
  int attrPos = element.indexOf(attribute + "=\"");
  if (attrPos == -1) return "";

  int startPos = attrPos + attribute.length() + 2;
  int endPos = element.indexOf("\"", startPos);
  if (endPos == -1) return "";

  return element.substring(startPos, endPos);
}

void loop()
{
  // put your main code here, to run repeatedly:
}
