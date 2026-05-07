#include "user_wifi.h"
#include "gui_paint.h"

// Function to initialize WiFi in station mode
void wifi_scan_init() {
    // Set WiFi to station mode (WIFI_STA) and ensure it is disconnected from any previously connected AP.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
  
    // Short delay to allow WiFi mode and disconnection settings to take effect
    delay(100);
}

// Function to check if a given SSID contains Chinese characters
bool contains_chinese(const char *ssid) {
    while (*ssid) {
        // Check if the highest bit of the character is set, indicating a non-ASCII character (possibly Chinese)
        if ((*ssid & 0x80) != 0) { 
            return true; // Return true if a non-ASCII character is found
        }
        ssid++; // Move to the next character
    }
    return false; // Return false if no non-ASCII characters are found
}

// Function to scan available WiFi networks and display results
void wifi_scan() {
    Serial0.println("Scan start");
    uint8_t chinese_num = 0; // Counter for networks with Chinese SSIDs

    // Start WiFi scan and get the number of networks found
    int n = WiFi.scanNetworks();
    Serial0.println("Scan done");
    if (n == 0) {
        // If no networks are found, print a message
        Serial0.println("no networks found");
    } else {
        // Print the number of networks found
        Serial0.print(n);
        Serial0.println(" networks found");
        Serial0.println("Nr | SSID                             | RSSI | CH | Encryption");

        // Iterate through each network found
        for (int i = 0; i < n; ++i) {
            // Print network details in a formatted table
            Serial0.printf("%2d", i + 1);
            Serial0.print(" | ");
            Serial0.printf("%-32.32s", WiFi.SSID(i).c_str());
            Serial0.print(" | ");
            Serial0.printf("%4ld", WiFi.RSSI(i));
            Serial0.print(" | ");
            Serial0.printf("%2ld", WiFi.channel(i));
            Serial0.print(" | ");
            switch (WiFi.encryptionType(i)) {
                case WIFI_AUTH_OPEN:            Serial0.print("open"); break;
                case WIFI_AUTH_WEP:             Serial0.print("WEP"); break;
                case WIFI_AUTH_WPA_PSK:         Serial0.print("WPA"); break;
                case WIFI_AUTH_WPA2_PSK:        Serial0.print("WPA2"); break;
                case WIFI_AUTH_WPA_WPA2_PSK:    Serial0.print("WPA+WPA2"); break;
                case WIFI_AUTH_WPA2_ENTERPRISE: Serial0.print("WPA2-EAP"); break;
                case WIFI_AUTH_WPA3_PSK:        Serial0.print("WPA3"); break;
                case WIFI_AUTH_WPA2_WPA3_PSK:   Serial0.print("WPA2+WPA3"); break;
                case WIFI_AUTH_WAPI_PSK:        Serial0.print("WAPI"); break;
                default:                        Serial0.print("unknown");
            }
            Serial0.println();

            // Limit display to the first 20 networks to fit the screen
            if (i < 20) {
                // Check if the SSID contains Chinese characters
                if (contains_chinese(WiFi.SSID(i).c_str())) {
                    chinese_num++; // Increment the counter for Chinese SSIDs
                    printf("Skipping SSID with Chinese characters: %s \r\n", WiFi.SSID(i).c_str());
                } else {
                    // Display the SSID on the screen
                    Paint_DrawString_EN(440, (i - chinese_num) * 24, WiFi.SSID(i).c_str(), &Font24, BLACK, WHITE);
                }
            }
            delay(10); // Add a short delay to avoid overwhelming the CPU
        }
    }
    Serial0.println("");

    // Free memory allocated for the scan results
    WiFi.scanDelete();
}
