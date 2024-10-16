#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

#include <MD_MAX72xx.h>
#include <EEPROM.h>
#include <SPI.h>

#define TEST_HOST "github.com"
#define MAX_COORDINATES 624

// Define hardware type, size, and pin connections
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4  // Number of displays connected in series

#define EEPROM_SIZE 512         // Define the size of EEPROM
#define GITHUB_USERNAME_ADDR 0  // Start address for GitHub username

#define DATA_PIN   D7      // GPIO 13 on ESP8266 (D7 on NodeMCU)
#define CS_PIN     D8      // GPIO 15 on ESP8266 (D8 on NodeMCU)
#define CLK_PIN    D5      // GPIO 14 on ESP8266 (D5 on NodeMCU)
#define RESET_PIN  D6      // GPIO 12 on ESP8266 (D6 on NodeMCU)

#define WIFI "Who stole my mom?"
#define PASSWORD "goose"

MD_MAX72XX display = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Function prototype
void makeHTTPRequest(int* count, int coordinates[MAX_COORDINATES][2]);

WiFiManager wm;                               // Global WiFiManager instance
WiFiManagerParameter custom_github_username;  // Parameter for GitHub username

WiFiClientSecure client;
int coordinates[MAX_COORDINATES][2];
int count = 0;

// Button press duration
unsigned long buttonPressStartTime = 0;
bool isButtonPressed = false;

void setup() {
  Serial.begin(230400);

  // Initialize button pin
  pinMode(RESET_PIN, INPUT_PULLUP);

  display.begin();  // Initialize the display
  display.clear();  // Clear any previous data

  EEPROM.begin(EEPROM_SIZE);  // Initialize EEPROM
  String storedUsername = readGitHubUsername();
  String storedWIFI = WIFI;

  // Check if a GitHub username is stored in EEPROM
  if (storedUsername.length() > 0) {
    Serial.println("Stored GitHub Username: " + storedUsername);

    // Display the stored username on the LED matrix
    displayTextOnMatrix("Hello, " + storedUsername);
  } else {
    Serial.println("No GitHub Username found in EEPROM.");
  }

  // Define the custom parameter for the GitHub username
  int customFieldLength = 40;  // Maximum length for the GitHub username input
  new (&custom_github_username) WiFiManagerParameter("github_username", "GitHub Username", "", customFieldLength, "required placeholder=\"Enter GitHub Username\"");

  // Add the custom parameter to WiFiManager
  wm.addParameter(&custom_github_username);
  wm.setSaveParamsCallback(saveParamCallback);  // Callback to save custom parameters

  Serial.print("Connecting to WiFi using WiFiManager");

  if (!wm.autoConnect(WIFI, PASSWORD)) {
    Serial.println("Failed to connect or hit timeout");
    displayTextOnMatrix("Connect to: " + storedWIFI);
    delay(3000);
    ESP.restart();  // Restart if it fails to connect
  } else {
    Serial.println("Connected to WiFi!");

    // Ensure the GitHub username is provided
    String githubUsername = readGitHubUsername();
    Serial.println(githubUsername);
    if (strlen(githubUsername.c_str()) == 0) {
      Serial.println("GitHub Username is required! Please try again.");
      wm.resetSettings();
      ESP.restart();  // Restart to reinitialize WiFiManager
    } else {
      Serial.println("GitHub Username: " + githubUsername);
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());

      saveGitHubUsername(githubUsername.c_str());  // Save username to EEPROM
      client.setInsecure();
      makeHTTPRequest(&count, coordinates);
    }
  }
}

void makeHTTPRequest(int* count, int coordinates[MAX_COORDINATES][2]) {
  if (!client.connect(TEST_HOST, 443)) {
    Serial.println("Connection failed");
    return;
  }

  Serial.println("Connected to server");
  String githubUsername = readGitHubUsername();
  String getRequest = "GET /users/" + githubUsername + "/contributions?from=2024-01-01&to=2024-12-31 HTTP/1.1\r\n";

  client.print(getRequest);
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
        int tdStartIndex = line.indexOf("<td");
        while (tdStartIndex != -1) {
          int tdEndIndex = line.indexOf(">", tdStartIndex);
          if (tdEndIndex != -1) {
            String tdElement = line.substring(tdStartIndex, tdEndIndex + 1);

            String dataLevel = extractAttribute(tdElement, "data-level=\"");
            if (dataLevel.length() > 0 && dataLevel != "0") {
              String id = extractAttribute(tdElement, "id=\"contribution-day-component-");
              if (id.length() > 0) {
                const char* id_cstr = id.c_str();
                char* dash_pos = strchr(const_cast<char*>(id_cstr), '-');
                if (dash_pos) {
                  *dash_pos = '\0';            // Null-terminate the x part
                  int x = atoi(id_cstr);       // Convert x part to integer
                  int y = atoi(dash_pos + 1);  // Convert y part to integer

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

  for (int i = 0; i < *count; i++) {
    display.setPoint(coordinates[i][0], (coordinates[i][1] - 6), true);
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

// Save parameter callback
void saveParamCallback() {
  Serial.println("[CALLBACK] saveParamCallback fired");
  String githubUsername = String(custom_github_username.getValue());
  Serial.println("GitHub Username = " + githubUsername);
  saveGitHubUsername(githubUsername.c_str());
}


void saveGitHubUsername(const char* username) {
  int i = 0;
  for (; i < strlen(username); i++) {
    EEPROM.write(GITHUB_USERNAME_ADDR + i, username[i]);  // Write username to EEPROM
  }
  EEPROM.write(GITHUB_USERNAME_ADDR + i, '\0');  // Null-terminate the string
  EEPROM.commit();                               // Save changes
  Serial.println("GitHub username saved to EEPROM.");
}

String readGitHubUsername() {
  char username[40];  // Adjust size as needed
  int i = 0;
  while (i < 40) {  // Reading username from EEPROM
    char c = EEPROM.read(GITHUB_USERNAME_ADDR + i);
    if (c == '\0') break;
    username[i] = c;
    i++;
  }
  username[i] = '\0';  // Null-terminate the string
  return String(username);
}

// Function to display text on the LED matrix across 4 displays
void displayTextOnMatrix(String text) {
  display.clear();  // Clear the display before showing the text
  int textLength = text.length();
  int maxDisplayChars = MAX_DEVICES * 8;  // Each display can show 8 columns

  // Scroll the text if it exceeds the available width of 4 displays
  for (int i = 0; i < textLength * 8 + maxDisplayChars; i++) {
    display.clear();  // Clear the display for each scroll step

    // Display the text with scrolling effect
    for (int j = 0; j < textLength; j++) {
      display.setChar((j * 8) - i, text[j]);  // Shift each character based on scroll position
    }

    display.update();  // Update the display to reflect changes
    delay(100);        // Adjust scrolling speed (lower for faster, higher for slower)
  }
}

// Function to reset WiFi credentials and GitHub username
void resetSettings() {
  Serial.println("Resetting settings...");

  // Reset WiFi settings
  wm.resetSettings();

  // Clear GitHub username from EEPROM
  for (int i = 0; i < 40; i++) {
    EEPROM.write(GITHUB_USERNAME_ADDR + i, 0);
  }
  EEPROM.commit();

  // Restart the device
  ESP.restart();
}

void loop() {
  int refetchCounter = 0;
  // Check button press
  if (digitalRead(RESET_PIN) == LOW) { // Button pressed (assuming active low)
    if (!isButtonPressed) {
      buttonPressStartTime = millis();
      isButtonPressed = true;
    }
    // Check if button is held for more than 5 seconds
    if (isButtonPressed && (millis() - buttonPressStartTime > 5000)) {
      resetSettings();
    }
  } else {
    isButtonPressed = false;
  }

  refetchCounter++;

  // Refetch Data after 60 secs
  if (refetchCounter == 60){
    Serial.println("Refetching Data");
    refetchCounter = 0;
    makeHTTPRequest(&count, coordinates);
  }
  delay(1000); // Adjust the loop interval
}
