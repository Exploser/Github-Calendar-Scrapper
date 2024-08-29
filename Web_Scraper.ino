

////
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

char ssid[] = "Kroneinc";        // your network SSID (name)
char password[] = "5197789927";  // your network key

WiFiClientSecure client;
#define TEST_HOST "github.com"
#define MAX_COORDINATES 624

// Function prototype
void makeHTTPRequest(int *count, int coordinates[MAX_COORDINATES][2]);

int coordinates[MAX_COORDINATES][2];
int count = 0;


void setup() {
  Serial.begin(230400);

  
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

  client.print("GET /users/Exploser/contributions?from=2024-07-31&to=2024-08-27 HTTP/1.1\r\n");
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
                  Serial.print("/nFound id: ");
                  Serial.println(id);

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
                      // Serial.println(y);
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
  Serial.println(coordinates[0][0]);
  Serial.println(*count);

  // parseResponse(response);
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
  // No repeated tasks
}
