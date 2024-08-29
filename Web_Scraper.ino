#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#include <MD_MAX72xx.h>
#include <SPI.h>

char ssid[] = "Kroneinc";        // your network SSID (name)
char password[] = "5197789927";  // your network key

WiFiClientSecure client;
#define TEST_HOST "github.com"
#define MAX_COORDINATES 624

// Define hardware type, size, and pin connections
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4  // Number of displays connected in series

#define DATA_PIN   D7  // GPIO 13 on ESP8266 (D7 on NodeMCU)
#define CS_PIN     D8  // GPIO 15 on ESP8266 (D8 on NodeMCU)
#define CLK_PIN    D5  // GPIO 14 on ESP8266 (D5 on NodeMCU)

MD_MAX72XX display = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Function prototype
void makeHTTPRequest(int *count, int coordinates[MAX_COORDINATES][2]);

int coordinates[MAX_COORDINATES][2];
int count = 0;


void setup() {
  Serial.begin(230400);

  display.begin();  // Initialize the display
  display.clear();  // Clear any previous data

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

  client.setInsecure();  // Disable SSL verification for testing
  makeHTTPRequest(&count, coordinates);
}

void makeHTTPRequest(int *count, int coordinates[MAX_COORDINATES][2]) {
  if (!client.connect(TEST_HOST, 443)) {
    Serial.println("Connection failed");
    return;
  }

  Serial.println("Connected to server");

  client.print("GET /users/Exploser/contributions?from=2024-01-01&to=2024-12-31 HTTP/1.1\r\n");
  // client.print("GET /Exploser?action=show&controller=profiles&tab=contributions&user_id=Exploser HTTP/1.1\r\n");
  client.print("Host: ");
  client.println(TEST_HOST);
  client.println("Connection: close");
  client.println();

  String response = "";
  bool startStoring = false;
  char buffer[256];

  // Read and process the response
  while (client.connected() || client.available()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');

      if (line.indexOf("<tbody>") != -1) {
        startStoring = true;
        Serial.println("Found <tbody> - starting to process data.");
      }

      if (startStoring) {
        // Check if the line contains a <td> tag
        int tdStartIndex = line.indexOf("<td");
        while (tdStartIndex != -1) {
          int tdEndIndex = line.indexOf(">", tdStartIndex);
          if (tdEndIndex != -1) {
            String tdElement = line.substring(tdStartIndex, tdEndIndex + 1);

            String dataLevel = extractAttribute(tdElement, "data-level=\"");
            if (dataLevel.length() > 0) {
              // Serial.print("Data Level: ");
              // Serial.print(dataLevel);

              if (dataLevel != "0") {
                // Extract the id attribute from the <td> element
                String id = extractAttribute(tdElement, "id=\"contribution-day-component-");
                if (id.length() > 0) {
                  // Serial.print("/nFound id: ");
                  // Serial.println(id);

                  // Convert id to C-style string
                  const char* id_cstr = id.c_str();

                  // Find the dash character
                  char* dash_pos = strchr(const_cast<char*>(id_cstr), '-');
                  if (dash_pos) {
                    *dash_pos = '\0';  // Null-terminate the x part
                    int x = atoi(id_cstr);          // Convert x part to integer
                    int y = atoi(dash_pos + 1);    // Convert y part to integer

                    // Store the coordinates in the array
                    if (*count < MAX_COORDINATES) {
                      coordinates[*count][0] = x;
                      coordinates[*count][1] = y;
                      (*count)++;
                    } else {
                      Serial.println("Reached maximum number of coordinates");
                      return;  // Array is full
                    }
                  }
                }
              }
            }

            // Move to the next <td> tag in the line
            tdStartIndex = line.indexOf("<td", tdEndIndex);
          } else {
            break;  // Exit loop if no closing > is found
          }
        }

        if (line.indexOf("</tbody>") != -1) {
          startStoring = false;  // Stop storing if you reach the end of <tbody>
        }
      }
    }
  }

  Serial.println("Finished processing response.");

   // Loop through all the stored coordinates and print them
  // Serial.println("Stored Coordinates:");
  for (int i = 0; i < *count; i++) {
    // Serial.print("Coordinate ");
    // Serial.print(i);
    // Serial.print(": (");
    // Serial.print(coordinates[i][0]);
    // Serial.print(", ");
    // Serial.print(coordinates[i][1]);
    // Serial.println(")");

    display.setPoint(coordinates[i][0],(coordinates[i][1] - 6), true);
  }

  Serial.println("Total number of coordinates:");
  Serial.println(*count);

}

// Helper function to extract attribute value
String extractAttribute(String element, String attribute) {
  String attributeValue = "";
  int attrStartIndex = element.indexOf(attribute);
  if (attrStartIndex != -1) {
    int valueStartIndex = attrStartIndex + attribute.length();
    int valueEndIndex = element.indexOf("\"", valueStartIndex);
    if (valueEndIndex != -1) {
      attributeValue = element.substring(valueStartIndex, valueEndIndex);
    }
  }
  return attributeValue;
}

void loop() {
  delay(60000);
  Serial.println("Refetching Data");
  makeHTTPRequest(&count, coordinates);
}
