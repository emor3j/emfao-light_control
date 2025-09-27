/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file wifi_portal.cpp
 * @brief Implementation of WiFiPortal class
 * 
 * This file implements the WiFiPortal class for ESP32 LED controller system.
 *
 * See wifi_portal.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include <ArduinoJson.h>

#include "wifi_portal.h"
#include "log.h"


WiFiPortal wifi_portal;

// === Constructor and Destructor ===

// Default constructor
WiFiPortal::WiFiPortal() : 
	status_(Status::IDLE),
	portal_start_time_(0),
	initialized_(false),
	connection_start_time_(0) {}

// Destructor
WiFiPortal::~WiFiPortal() {
	stopPortal();
}

// === Getters ===

bool WiFiPortal::isActive() const {
	return (status_ == Status::ACTIVE || status_ == Status::CONNECTING || status_ == Status::SUCCESS);
}

std::string WiFiPortal::getStatusString() const {
	switch (status_) {
		case Status::IDLE:       return "Idle";
		case Status::STARTING:   return "Starting";
		case Status::ACTIVE:     return "Active";
		case Status::CONNECTING: return "Connecting";
		case Status::SUCCESS:    return "Success";
		case Status::FAILED:     return "Failed";
		case Status::TIMEOUT:    return "Timeout";
		default:                 return "Unknown";
	}
}

// === Portal lifecycle ===

bool WiFiPortal::initialize(const Config& config, ConfigCallback callback) {
	if (!callback) {
		LOG_ERROR("[CONFIGAP] Error: No callback provided\n");
		return false;
	}
	
	config_ = config;
	config_callback_ = callback;
	initialized_ = true;
	
	LOG_INFO("[CONFIGAP] WiFi Portal initialized\n");
	LOG_INFO("[CONFIGAP] AP SSID: %s\n", config_.ap_ssid.c_str());
	
	return true;
}

bool WiFiPortal::startPortal() {
	if (!initialized_) {
		LOG_ERROR("[CONFIGAP] Error: Portal not initialized\n");
		return false;
	}
	
	if (status_ == Status::ACTIVE) {
		LOG_INFO("[CONFIGAP] Portal already active\n");
		return true;
	}
	
	LOG_INFO("[CONFIGAP] Starting configuration portal...\n");
	status_ = Status::STARTING;
	
	// Start Access Point avec configuration plus explicite
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAPdisconnect(true); // Force clean start
	delay(100);

	bool ap_started = WiFi.softAP(
		config_.ap_ssid.c_str(),
		config_.ap_password.c_str(),
		1,     // Channel
		0,     // Hidden (0 = visible)
		4,     // Max connections
		false  // No beacon delay
	);

	if (!ap_started) {
		LOG_ERROR("[CONFIGAP] Failed to start Access Point\n");
		status_ = Status::FAILED;
		return false;
	}

	// Configure AP IP - signature correcte
	IPAddress local_ip(192, 168, 4, 1);
	IPAddress gateway(192, 168, 4, 1);  // Gateway = nous-même
	IPAddress subnet(255, 255, 255, 0);
	IPAddress dhcp_start(192, 168, 4, 2); // DHCP commence à .2

	if (!WiFi.softAPConfig(local_ip, gateway, subnet, dhcp_start)) {
		LOG_ERROR("[CONFIGAP] Failed to configure AP network\n");
		status_ = Status::FAILED;
		return false;
	}

	LOG_INFO("[CONFIGAP] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
	LOG_INFO("[CONFIGAP] AP Gateway: %s\n", gateway.toString().c_str());
	LOG_INFO("[CONFIGAP] DHCP range: %s - %s\n", dhcp_start.toString().c_str(), "192.168.4.254");

	portal_start_time_ = millis();
	status_ = Status::ACTIVE;

	LOG_INFO("[CONFIGAP] Configuration portal started successfully\n");
	LOG_INFO("[CONFIGAP] Connect to WiFi: %s\n", config_.ap_ssid.c_str());
	LOG_INFO("[CONFIGAP] Password: %s\n", config_.ap_password.c_str());
	LOG_INFO("[CONFIGAP] Portal URL: http://%s\n", WiFi.softAPIP().toString().c_str());
	
	return true;
}

void WiFiPortal::stopPortal() {
	LOG_INFO("[CONFIGAP] Stopping configuration portal...\n");
	
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_STA);
	
	status_ = Status::IDLE;
	LOG_INFO("[CONFIGAP] Configuration portal stopped\n");
}

void WiFiPortal::handlePortal() {
	if (!isActive()) {
		return;
	}
	
	// Log clients connectés avec plus de détails
	static unsigned long last_client_check = 0;
	if (millis() - last_client_check > 5000) {
		uint8_t client_count = WiFi.softAPgetStationNum();
		if (client_count > 0) {
			LOG_INFO("[CONFIGAP] %d client(s) connected - AP IP: %s\n", 
			        client_count, WiFi.softAPIP().toString().c_str());
		}
		last_client_check = millis();
	}
	
	// Handle connection testing
	if (status_ == Status::CONNECTING) {
		unsigned long elapsed = millis() - connection_start_time_;
		
		if (WiFi.status() == WL_CONNECTED) {
			LOG_INFO("[CONFIGAP] WiFi connection successful\n");
			
			// Call callback with successful configuration
			if (config_callback_(pending_ssid_, pending_password_)) {
				status_ = Status::SUCCESS;
				LOG_INFO("[CONFIGAP] Configuration accepted and saved\n");
				
				// Keep portal active but in success state
				LOG_INFO("[CONFIGAP] Portal remains active for backup access\n");
			} else {
				LOG_ERROR("[CONFIGAP] Configuration rejected by callback\n");
				status_ = Status::FAILED;
				WiFi.disconnect();
			}
			
		} else if (elapsed > CONNECTION_TIMEOUT_MS) {
			LOG_ERROR("[CONFIGAP] WiFi connection timeout\n");
			status_ = Status::ACTIVE; // Return to active state
			WiFi.disconnect();
		}
	}
}

// === Private functions ===

bool WiFiPortal::hasTimedOut() const {
	// Portal no longer times out
	return false;
}