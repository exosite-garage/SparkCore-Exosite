//*****************************************************************************
//
// Copyright (c) 2015 Exosite LLC.  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:

//  * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of Exosite LLC nor the names of its contributors may
//    be used to endorse or promote products derived from this software 
//    without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
//*****************************************************************************

#include "MQTT-TLS.h"
#include "Exosite.h"
#include <string>

/*==============================================================================
*  Declare Prototypes
*=============================================================================*/
void callback(char *topic, byte *payload, unsigned int length);
void split_topic(char *topic, String &context, String &endpoint);
const char * deviceConfigExoSense();

// Modify the host value based on your specific IoT Connector in the Exosite cloud
#define host "a1y9xqdkuy1vk0000"
char* domain = host".m2.exosite.io";

// Create MQTT-TLS client and use with the Exosite library
MQTT client(domain, 8883, callback);
Exosite exosite(&client);

// To hold MAC Address (DeviceID)
byte macData[6];
char macString[18];

// Used for syncing board time on boot (see setup())
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastSync = millis();

// Used when publishing data to Exosite cloud (add more in this same way if needed)
char provision_topic[] = "$provision/";
const char config_topic[] = "$resource/config_io";
const char data_topic[] = "$resource/data_in";
const char batch_topic[] = "$resource/";
const char record_topic[] = "$resource.batch";

uint32_t freemem;

/*==============================================================================
*  setup
*
*  On-boot operations
*=============================================================================*/
void setup() {
    Serial.begin(9600);
    Serial.println(F("\n======================== SETUP =========================="));

    freemem = System.freeMemory();
    Serial.print("Free Memory (TOP SETUP): ");
    Serial.println(freemem);

    /*--------------------------------------------------------------------------
    *   Uncomment following line if needing to clear previously-stored token
    *   Make sure identity is available in the cloud
    *-------------------------------------------------------------------------*/
    // exosite.clearAuth();

    Serial.printf("Configured domain: %s\n", domain);

    // Sync time and enable RGB control
    if (millis() - lastSync > ONE_DAY_MILLIS) {
        Particle.syncTime();
        lastSync = millis();
    }
    RGB.control(true);

    Serial.println(F("\n===================== PROVISIONING ======================"));

    // Derive NIC MAC Address
    WiFi.macAddress(macData);
    snprintf(macString, 18, "%02X%02X%02X%02X%02X%02X",
             macData[5], macData[4], macData[3], macData[2], macData[1], macData[0]);
    Serial.printf("Identity (MAC Address): %s\n", macString);

    // Append MAC Address to provision topic to act as Device Identifier
    strcat(provision_topic, macString);

    freemem = System.freeMemory();
    Serial.print("Free Memory (BEFORE PROVISION): ");
    Serial.println(freemem);

    // If no token is found in EEPROM, provision with Exosite
    if (!exosite.isProvisioned()) {
        Serial.println("No token found, starting provisioning sequence...");
        exosite.provision(provision_topic);
    }
    else {
        Serial.println("Token found in EEPROM...");
    }

    freemem = System.freeMemory();
    Serial.print("Free Memory (AFTER PROVISION): ");
    Serial.println(freemem);

    Serial.println(F("\n===================== REPORT CONFIG ====================="));

    // Report config_io value - data channel definition(s) - to Exosite
    if (!exosite.isConnected()) {
        exosite.connect();
    }

    freemem = System.freeMemory();
    Serial.print("Free Memory (AFTER CONNECT): ");
    Serial.println(freemem);

    //
    // NOTE: HAD TO MODIFY UNDERLYING PUBLISH METHOD TO EXPAND SUPPORTED PAYLOAD SIZE
    //
    exosite.publish(config_topic, deviceConfigExoSense());

    freemem = System.freeMemory();
    Serial.print("Free Memory (END SETUP): ");
    Serial.println(freemem);

    Serial.println(F("\n========================= LOOP =========================="));
}

/*==============================================================================
*  loop
*
*  main continuous loop - handle sensor reading, data reporting, etc.
*=============================================================================*/
void loop() {
    Serial.println(F("Top of the loop..."));
    freemem = System.freeMemory();
    Serial.print("Free Memory (TOP LOOP): ");
    Serial.println(freemem);

    // Reconnect if client becomes disconnected at any point
    if (!exosite.isConnected()) {
        exosite.connect();
    }

    /*--------- START CUSTOM CODE ---------*/

    /*---------- END CUSTOM CODE ---------*/

    // Check buffer for queued messages from cloud topics (subscriptions)
    exosite.checkForMessage();

    // Control loop frequency (ms)
    delay(10000);
    freemem = System.freeMemory();
    Serial.print("Free Memory (END LOOP): ");
    Serial.println(freemem);
}

/*==============================================================================
*  callback
*
*  Handle received data (from subscribed topics)
*=============================================================================*/
void callback(char *topic, byte *payload, unsigned int length) {
    Serial.println(F("\n======================= CALLBACK ========================"));
    freemem = System.freeMemory();
    Serial.print("Free Memory (START CALLBACK): ");
    Serial.println(freemem);

    // Derive actionable topic components
    String context;
    String endpoint;
    String timestamp;
    split_topic(topic, context, endpoint, timestamp);
    Serial.printf("Full Topic: %s\n", topic);
    Serial.println("Context: " + context);
    Serial.println("Endpoint: " + endpoint);
    Serial.println("Timestamp: " + timestamp);

    // Derive actionable message value
    char message_char[length + 1];
    memcpy(message_char, payload, length);
    message_char[length] = NULL;
    String message(message_char);

    freemem = System.freeMemory();
    Serial.print("Free Memory (MID CALLBACK): ");
    Serial.println(freemem);

    // Handle provision callback - do not modify
    if (context == "$provision") {
        Serial.println(F("Authentication token recieved - storing in non-volatile memory..."));

        // Store received auth token in EEPROM
        exosite.useAuth(message_char);
    }
    /*--------- START CUSTOM HANDLING CODE ---------*/
    else {
        Serial.println(F("Handling received message..."));

        // Note: Must report-back a value to resolve cloud sync-state
        exosite.publish(context + "/" + endpoint, message);
    }
    /*---------- END CUSTOM HANDLING CODE ---------*/

    freemem = System.freeMemory();
    Serial.print("Free Memory (END CALLBACK): ");
    Serial.println(freemem);

    Serial.println(F("===================== END CALLBACK ======================\n"));
}

/*==============================================================================
*  split_topic
*
*  Split topic string into actionable parts
*=============================================================================*/
void split_topic(char *topic, String &context, String &endpoint, String &timestamp) {
    bool first = true;
    bool second = true;
    char *part;
    part = strtok(topic, "/");
    while (part != NULL)
    {
        if (first) {
            context += part;
            first = false;
        }
        else if (second) {
            endpoint += part;
            second = false;
        }
        else {
            timestamp += part;
        }
        part = strtok(NULL, "/");
    }
}

/*==============================================================================
*  deviceConfigExosense
*
*  Build and return device config (config_io JSON string)
*=============================================================================*/
const char * deviceConfigExoSense() {
     const char *config = "{"
        "\"channels\":{"
            "\"001\":{"
                "\"display_name\":\"Temperature\","
                "\"description\":\"Temperature\","
                "\"properties\":{"
                    "\"data_type\":\"TEMPERATURE\","
                    "\"data_unit\":\"DEG_FAHRENHEIT\","
                    "\"precision\":2"
                "},"
                "\"protocol_config\":{"
                    "\"report_rate\":5000,"
                    "\"timeout\":60000"
                "}"
            "},"
            "\"002\":{"
                "\"display_name\":\"Humidity\","
                "\"description\":\"Humidity\","
                "\"properties\":{"
                    "\"data_type\":\"HUMIDITY\","
                    "\"data_unit\":\"PERCENT\","
                    "\"precision\":2"
                "},"
                "\"protocol_config\":{"
                    "\"report_rate\":5000,"
                    "\"timeout\":60000"
                "}"
            "},"
            "\"003\":{"
                "\"display_name\":\"Pressure\","
                "\"description\":\"Pressure\","
                "\"properties\":{"
                    "\"data_type\":\"PRESSURE\","
                    "\"data_unit\":\"PSI\","
                    "\"precision\":2"
                "},"
                "\"protocol_config\":{"
                    "\"report_rate\":5000,"
                    "\"timeout\":60000"
                "}"
            "}"
        "}"
    "}";

    return config;
}
