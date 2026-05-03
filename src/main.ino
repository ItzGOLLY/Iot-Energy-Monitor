
---

```cpp
/**
 * IoT-Based Energy Monitoring System
 * 
 * Reads energy meter data via Modbus RTU over RS485
 * and publishes to Zoho IoT Cloud via MQTT over TLS.
 * 
 * Hardware: ESP32 + Schneider Conzerv EM6400NG + RS485 Module
 * 
 * @author Aarush (itzgolly)
 * @organization Zoho Corporation (Internship Project)
 */

#include <WiFi.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <zoho-iot-client.h>
#include "certificate.h"
#include <WiFiManager.h>
#include <ModbusMaster.h>
#include <Preferences.h>

// ==================== CONFIGURATION ====================

#define RESET_PIN 0           // GPIO0 for credential reset
#define UPDATE_INTERVAL 5000  // Data publish interval (ms)

// Buffer sizes for credentials
char WIFI_SSID[123] = "";
char WIFI_PASS[123] = "";
char MQTT_USERNAME[123] = "";
char MQTT_PASSWORD[123] = "";

// ==================== OBJECTS ====================

Preferences preferences;
WiFiManager wifiManager;
WiFiClientSecure espClient;
ZohoIOTClient zClient(&espClient, true);
ModbusMaster node;

bool shouldSaveConfig = false;

// Register addresses for Schneider EM6400NG
const uint16_t REGISTERS[] = {
    3109,  // Frequency
    2999,  // Current
    3019,  // Voltage AB (L-L)
    3027,  // Voltage AN (L-N)
    3069,  // Apparent Power
    3061,  // Reactive Power
    3077,  // Power Factor
    3053,  // Active Power
    3769   // Peak Demand Power
};

const char* LABELS[] = {
    "Frequency",
    "Current",
    "VoltageAB",
    "VoltageAN",
    "ApparentPower",
    "ReactivePower",
    "PowerFactor",
    "ActivePower",
    "PeakDemandPower"
};

const uint8_t NUM_REGISTERS = sizeof(REGISTERS) / sizeof(REGISTERS[0]);

// ==================== CALLBACKS ====================

void saveConfigCallback() {
    Serial.println("[CONFIG] Portal save triggered");
    shouldSaveConfig = true;
}

// ==================== CREDENTIAL MANAGEMENT ====================

void loadCredentials() {
    preferences.begin("net-creds", false);
    
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("wifipass", "");
    String mqtt_user = preferences.getString("mqttuser", "");
    String mqtt_pass = preferences.getString("mqttpass", "");
    
    preferences.end();
    
    strlcpy(WIFI_SSID, ssid.c_str(), sizeof(WIFI_SSID));
    strlcpy(WIFI_PASS, pass.c_str(), sizeof(WIFI_PASS));
    strlcpy(MQTT_USERNAME, mqtt_user.c_str(), sizeof(MQTT_USERNAME));
    strlcpy(MQTT_PASSWORD, mqtt_pass.c_str(), sizeof(MQTT_PASSWORD));
    
    Serial.println("[CREDENTIALS] Loaded from NVS");
}

void saveCredentials() {
    preferences.begin("net-creds", false);
    
    preferences.putString("ssid", WIFI_SSID);
    preferences.putString("wifipass", WIFI_PASS);
    preferences.putString("mqttuser", MQTT_USERNAME);
    preferences.putString("mqttpass", MQTT_PASSWORD);
    
    preferences.end();
    
    Serial.println("[CREDENTIALS] Saved to NVS");
}

// ==================== WIFI ====================

void setupWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    
    Serial.print("[WIFI] Connecting to ");
    Serial.println(WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connected!");
        Serial.print("[WIFI] IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[WIFI] Connection failed!");
    }
}

// ==================== MODBUS ====================

/**
 * Read 32-bit float from two consecutive holding registers
 * Handles byte order conversion for Schneider EM6400NG
 */
float readFloatHex(uint16_t regAddress, const char* label) {
    uint8_t result = node.readHoldingRegisters(regAddress, 2);
    
    if (result == node.ku8MBSuccess) {
        uint16_t reg1 = node.getResponseBuffer(0);
        uint16_t reg2 = node.getResponseBuffer(1);
        
        union {
            uint8_t b[4];
            float f;
        } converter;
        
        // Byte order: [reg2_LSB, reg2_MSB, reg1_LSB, reg1_MSB]
        converter.b[0] = reg2 & 0xFF;
        converter.b[1] = (reg2 >> 8) & 0xFF;
        converter.b[2] = reg1 & 0xFF;
        converter.b[3] = (reg1 >> 8) & 0xFF;
        
        float value = converter.f;
        
        Serial.print("[MODBUS] ");
        Serial.print(label);
        Serial.print(": ");
        Serial.println(value, 4);
        
        return value;
    } else {
        Serial.print("[MODBUS] ERROR reading ");
        Serial.print(label);
        Serial.print(" (reg ");
        Serial.print(regAddress);
        Serial.print("): 0x");
        Serial.println(result, HEX);
        return NAN;
    }
}

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("  IoT Energy Monitor - Zoho IoT");
    Serial.println("  Booting up...");
    Serial.println("========================================\n");
    
    // Initialize Modbus on Serial2
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    node.begin(1, Serial2);  // Slave ID = 1
    
    // Load saved credentials
    loadCredentials();
    
    // Configure WiFiManager parameters
    WiFiManagerParameter custom_wifi_ssid("ssid", "WiFi SSID", WIFI_SSID, 123);
    WiFiManagerParameter custom_wifi_pass("pass", "WiFi Password", WIFI_PASS, 123);
    WiFiManagerParameter custom_mqtt_user("mqttuser", "MQTT Username", MQTT_USERNAME, 123);
    WiFiManagerParameter custom_mqtt_pass("mqttpass", "MQTT Password", MQTT_PASSWORD, 123);
    
    wifiManager.addParameter(&custom_wifi_ssid);
    wifiManager.addParameter(&custom_wifi_pass);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
    
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    
    // Attempt auto-connect with saved credentials
    if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
        Serial.println("[WIFI] Failed to connect, restarting...");
        delay(3000);
        ESP.restart();
    }
    
    // Copy updated parameters from portal
    strlcpy(WIFI_SSID, custom_wifi_ssid.getValue(), sizeof(WIFI_SSID));
    strlcpy(WIFI_PASS, custom_wifi_pass.getValue(), sizeof(WIFI_PASS));
    strlcpy(MQTT_USERNAME, custom_mqtt_user.getValue(), sizeof(MQTT_USERNAME));
    strlcpy(MQTT_PASSWORD, custom_mqtt_pass.getValue(), sizeof(MQTT_PASSWORD));
    
    // Save if portal was used
    if (shouldSaveConfig) {
        saveCredentials();
    }
    
    // Setup connections
    setupWiFi();
    espClient.setCACert(root_ca);
    
    // Initialize Zoho IoT client
    zClient.init(MQTT_USERNAME, MQTT_PASSWORD);
    zClient.connect();
    
    Serial.println("\n========================================");
    Serial.println("  System Ready!");
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("========================================\n");
}

// ==================== LOOP ====================

void loop() {
    // Ensure WiFi connection
    setupWiFi();
    
    // Ensure MQTT connection
    zClient.reconnect();
    
    // Check reset button (GPIO0, active LOW)
    pinMode(RESET_PIN, INPUT_PULLUP);
    if (digitalRead(RESET_PIN) == LOW) {
        Serial.println("[RESET] Button pressed! Clearing credentials...");
        
        preferences.begin("net-creds", false);
        preferences.clear();
        preferences.end();
        
        WiFi.disconnect(true, true);
        
        Serial.println("[RESET] Restarting into config mode...");
        delay(1000);
        ESP.restart();
    }
    
    // Publish data at interval
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = millis();
        
        if (zClient.isConnected()) {
            Serial.println("\n[DATA] Reading sensors...");
            
            for (uint8_t i = 0; i < NUM_REGISTERS; i++) {
                float value = readFloatHex(REGISTERS[i], LABELS[i]);
                
                if (!isnan(value)) {
                    zClient.addDataPointNumber(LABELS[i], value);
                }
            }
            
            String payload = zClient.getPayload();
            Serial.print("[MQTT] Payload: ");
            Serial.println(payload);
            
            if (zClient.dispatch() == zClient.SUCCESS) {
                Serial.println("[MQTT] Published successfully!");
            } else {
                Serial.println("[MQTT] Publish failed!");
            }
        } else {
            Serial.println("[MQTT] Not connected, skipping publish");
        }
    }
    
    zClient.zyield();
}
