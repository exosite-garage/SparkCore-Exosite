//*****************************************************************************
//
// exosite.cpp - Prototypes for the Exosite Cloud API
//
// Copyright (c) 2015 Exosite LLC.    All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//        this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//        notice, this list of conditions and the following disclaimer in the
//        documentation and/or other materials provided with the distribution.
//    * Neither the name of Exosite LLC nor the names of its contributors may
//        be used to endorse or promote products derived from this software
//        without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
//*****************************************************************************

#include "Exosite.h"
#include <string.h>
#include <Arduino.h>


/*==============================================================================
*  Exosite
*
*  constructor for Exosite class
*=============================================================================*/
Exosite::Exosite(MQTT *client_ref)
{
    client = client_ref;
    client->setMaxPacketSize(5000);
}

Exosite::Exosite(MQTT *client_ref, char *token_ref)
{
    client = client_ref;
    client->setMaxPacketSize(5000);
    strncpy(token, token_ref, 41);
}

/*==============================================================================
*  provision
*
*  Provision on Exosite Platform, activate device and get cik.
*=============================================================================*/
bool Exosite::provision(const char *topic) {
    // Ensure TLS enabled for the MQTT-TLS client - required for communicating with Exosite
    client->enableTls(DIGICERT_GLOBAL_ROOT_CA_PEM, sizeof(DIGICERT_GLOBAL_ROOT_CA_PEM));

    timeout_time = 0;
    time_now = 0;
    timeout = 2000; // 2s

    if (!client->isConnected()) {
        Serial.print(F("No existing connection - opening one (to provision)... "));
        client->connect("");
    }

    if (client->isConnected()) {
        Serial.println(F("connected!"));

        #ifdef EXOSITEDEBUG
            Serial.printf("Publishing empty string to: %s\n", topic);
        #endif

        // Send provision request using Exosite MQTT Device API (provision)
        client->publish(topic, "");

        #ifdef EXOSITEDEBUG
            Serial.println(F("Provision publish complete..."));
        #endif

        // Provide time for response from server (auth token)
        timeout_time = millis() + timeout;
        while (timeout_time > time_now) {
            client->loop();
            time_now = millis();
        }
    }
    else {
        Serial.println(F("\nError: Failed to open connection to Exosite (to provision)..."));
        return false;
    }

    // Disconnect - connection parameters are unique for provisioning
    client->disconnect();

    #ifdef EXOSITEDEBUG
        Serial.println(F("End of provision sequence"));
    #endif

    return true;
}

/*==============================================================================
*  connect
*
*  Establish persistent connection
*=============================================================================*/
bool Exosite::connect() {
    // Ensure TLS enabled for the MQTT-TLS client - required for communicating with Exosite
    client->enableTls(DIGICERT_GLOBAL_ROOT_CA_PEM, sizeof(DIGICERT_GLOBAL_ROOT_CA_PEM));

    if (!client->isConnected()) {
        Serial.print(F("No existing connection - opening one... "));
        client->connect("", "", token);
    }

    if (client->isConnected()) {
        Serial.println(F("connected!"));
    }
    else {
        Serial.println(F("\nError: Failed to open connection to Exosite..."));
        return false;
    }

    return true;
}

/*==============================================================================
*  publish
*
*  Publish specified value to specified topic
*=============================================================================*/
bool Exosite::publish(const char *topic, const char *payload) {
    #ifdef EXOSITEDEBUG
        Serial.println(F("Attempting to publish message..."));
    #endif

    if (!client->isConnected()) {
        connect();
    }

    if (client->isConnected()) {
        #ifdef EXOSITEDEBUG
            Serial.printf("Topic (%i): %s\n", strlen(topic), topic);
            Serial.printf("Payload (%i): %s\n", strlen(payload), payload);
        #endif

        // Send request using Exosite MQTT Device API (general publish)
        client->publish(topic, payload);

        #ifdef EXOSITEDEBUG
            Serial.println(F("Data publish complete..."));
        #endif
    }
    else {
        Serial.println(F("\nError: Unable to publish message..."));
        return false;
    }
    return true;
}

/*==============================================================================
*  checkForMessage
*
*  Pull any queued subscription messages from NIC buffer
*=============================================================================*/
void Exosite::checkForMessage() {
    Serial.println(F("Checking for queued messages..."));
    if (!client->isConnected()) {
        connect();
    }

    if (client->isConnected()) {
        Serial.printf("Check-In Loop: %i\n", client->loop());
    }
    else {
        Serial.println(F("\nError: Unable to check for messages..."));
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*  Utility Procedures
*
/*==============================================================================
*  saveNVCIK
*
*  Write the CIK to EEPROM
*=============================================================================*/
bool Exosite::saveNVCIK() {
    #ifdef EXOSITEDEBUG
        Serial.printf("Storing token: %s\n", token);
    #endif

    EEPROM.put(TOKEN_EEPROM_ADDRESS, token);
    return true;
}

/*==============================================================================
*  fetchNVCIK
*
*  Fetch the CIK from EEPROM
*=============================================================================*/
bool Exosite::fetchNVCIK() {
    Serial.println(F("Checking for stored token..."));
    char temp[41];

    EEPROM.get(TOKEN_EEPROM_ADDRESS, temp);

    if (!isToken(temp)) {
        Serial.printf("Retrieved non-token data: %s\n", temp);
        return false;
    }
    strncpy(token, temp, 40);

    #ifdef EXOSITEDEBUG
        Serial.printf("Found token: %s\n", token);
    #endif

    return true;
}

/*==============================================================================
*  isProvisioned
*
*  Fetch the CIK from EEPROM
*=============================================================================*/
bool Exosite::isProvisioned() {
    return fetchNVCIK();
}

/*==============================================================================
*  isConnected
*
*  Verify current connections state
*=============================================================================*/
bool Exosite::isConnected() {
    return client->isConnected();
}

/*==============================================================================
*  isToken
*
*  Fetch the CIK from EEPROM
*=============================================================================*/
bool Exosite::isToken(char *val) {
    for(int i = 0; i < 40; i++) {
        if (!((val[i] >= '0' && val[i] <= '9') ||
              (val[i] >= 'A' && val[i] <= 'Z') ||
              (val[i] >= 'a' && val[i] <= 'z'))) {

            #ifdef EXOSITEDEBUG
                Serial.printf("Proposed token failed validation: %s\n", val);
                Serial.printf("Invalid character: %c\n", val[i]);
            #endif

            return false;
        }
    }
    return true;
}

/*==============================================================================
*  useAuth
*
*  Store valid token
*=============================================================================*/
bool Exosite::useAuth(char auth[41]) {
    if (isToken(auth)) {
        strcpy(token, auth);
        if (saveNVCIK()) {
            #ifdef EXOSITEDEBUG
                Serial.printf("Token (Stored): %s\n", token);
            #endif

            return true;
        }
        Serial.printf("Error: Failed to store token!");
    }
    return false;
}

/*==============================================================================
*  clearAuth
*
*  Clear EEPROM of any stored values (e.g. Auth Token)
*=============================================================================*/
void Exosite::clearAuth() {
    Serial.println(F("ALERT: CLEARING ALL EEPROM"));
    EEPROM.clear();
}
