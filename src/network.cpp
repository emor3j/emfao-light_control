/*
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file network.cpp
 * @brief Implementation of NetworkManager class
 * 
 * This file implements the network manager that handles
 * WiFi configuration and connection.
 *
 * See network.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include "network.h"
#include "log.h"
#include "storage.h"
#include "wifi_portal.h"

// Global instance
std::unique_ptr<NetworkManager> network_manager;

// === Constructor and Destructor ===

// Default constructor
NetworkManager::NetworkManager() :
	is_connected_(false),
	was_connected_(false),
	last_connection_check_(0),
	initialized_(false),
	max_connection_attempts_(3),
	current_attempt_count_(0),
	portal_started_(false) {
	
	// Set up credentials callback to use storage manager
	credentials_callback_ = [](const Credentials& credentials) -> bool {
		if (storage_manager) {
			return storage_manager->save_wifi_credentials(credentials.ssid, credentials.password);
		}
		return false;
	};
}

// Destructor
NetworkManager::~NetworkManager() {
	if (initialized_) {
		disconnect();
	}
}

// === Getters ===

std::string NetworkManager::getIPAddress() const {
	if (!isConnected()) {
		return "";
	}
	return WiFi.localIP().toString().c_str();
}

int32_t NetworkManager::getSignalStrength() const {
	if (!isConnected()) {
		return 0;
	}
	return WiFi.RSSI();
}

std::string NetworkManager::getMACAddress() const {
	return WiFi.macAddress().c_str();
}

std::string NetworkManager::getCurrentSSID() const {
	if (!isConnected()) {
		return "";
	}
	return WiFi.SSID().c_str();
}

// === Other functions ===

bool NetworkManager::initialize(const Credentials& credentials, int timeout_seconds) {
	return initialize(credentials, nullptr, timeout_seconds);
}

bool NetworkManager::initialize(const Credentials& credentials, CredentialsCallback callback, int timeout_seconds) {
	if (!credentials.isValid()) {
		LOG_ERROR("[NETWORKMGR] Error: Invalid WiFi credentials provided\n");
		return false;
	}
	
	credentials_ = credentials;
	credentials_callback_ = callback;
	initialized_ = true;
	current_attempt_count_ = 0;
	
	LOG_INFO("[NETWORKMGR] Connecting to WiFi...\n");
	LOG_INFO("[NETWORKMGR] SSID: %s\n", credentials_.ssid.c_str());
	
	WiFi.mode(WIFI_STA);
	
	bool success = attemptConnection(timeout_seconds);
	if (success) {
		is_connected_ = true;
		was_connected_ = true;
		current_attempt_count_ = 0; // Reset attempt counter on success
		LOG_INFO("[NETWORKMGR] WiFi connected successfully\n");
		printStatus();
		
		// Start AP portal
		if (!portal_started_) {
			startConfigurationPortal();
		}
	} else {
		is_connected_ = false;
		current_attempt_count_++;
		LOG_ERROR("[NETWORKMGR] WiFi connection failed\n");
		LOG_ERROR("[NETWORKMGR] Please check your WiFi credentials and try again.\n");
	}
	
	return success;
}

bool NetworkManager::checkConnection(int reconnect_timeout_seconds) {
	if (!initialized_) {
		LOG_WARNING("[NETWORKMGR] Warning: NetworkManager not initialized\n");
		return false;
	}
	
	// Handle configuration portal if needed
	if (wifi_portal.isActive()) {
		wifi_portal.handlePortal();
		
		// Check if we got a successful connection through portal
		if (wifi_portal.getStatus() == WiFiPortal::Status::SUCCESS) {
			is_connected_ = true;
			current_attempt_count_ = 0;
			return true;
		}
	}
	
	// Start portal if not already started
	if (!portal_started_) {
		startConfigurationPortal();
	}
	
	last_connection_check_ = millis();
	bool currently_connected = (WiFi.status() == WL_CONNECTED);
	
	if (!currently_connected) {
		if (is_connected_) {
			LOG_WARNING("[NETWORKMGR] WiFi connection lost. Attempting to reconnect...\n");
			logConnectionChange(false);
		}
		
		WiFi.disconnect();
		bool reconnected = attemptConnection(reconnect_timeout_seconds);
		
		if (reconnected) {
			logConnectionChange(true);
			LOG_INFO("[NETWORKMGR] WiFi reconnected successfully\n");
			LOG_INFO("[NETWORKMGR] IP Address: %s\n", WiFi.localIP().toString().c_str());
			current_attempt_count_ = 0; // Reset counter on success
		} else {
			current_attempt_count_++;
			LOG_ERROR("[NETWORKMGR] Reconnection failed. Attempt: %d/%d\n",
				current_attempt_count_, max_connection_attempts_);
		}
		
		is_connected_ = reconnected;
	} else {
		if (!is_connected_) {
			logConnectionChange(true);
			LOG_INFO("[NETWORKMGR] WiFi connection restored\n");
		} else {
			LOG_INFO("[NETWORKMGR] WiFi connection OK\n");
		}
		is_connected_ = true;
		current_attempt_count_ = 0; // Reset counter when connected
	}
	
	return is_connected_;
}

bool NetworkManager::isConnected() const {
	return is_connected_ && (WiFi.status() == WL_CONNECTED);
}

void NetworkManager::disconnect() {
	if (initialized_) {
		WiFi.disconnect();
		is_connected_ = false;
		LOG_INFO("[NETWORKMGR] WiFi disconnected\n");
	}
}

bool NetworkManager::updateCredentials(const Credentials& credentials) {
	if (!credentials.isValid()) {
		LOG_ERROR("[NETWORKMGR] Error: Invalid credentials provided\n");
		return false;
	}
	
	LOG_INFO("[NETWORKMGR] Updating WiFi credentials for: %s\n", credentials.ssid.c_str());
	
	// Disconnect current connection
	if (is_connected_) {
		WiFi.disconnect();
		delay(500);
	}
	
	// Update stored credentials
	credentials_ = credentials;
	current_attempt_count_ = 0; // Reset attempt counter for new credentials
	
	// Attempt connection with new credentials
	bool success = attemptConnection(30); // 30 second timeout for new credentials
	
	if (success) {
		// Save credentials if callback is available
		if (credentials_callback_) {
			if (credentials_callback_(credentials_)) {
				LOG_INFO("[NETWORKMGR] WiFi credentials saved successfully\n");
			} else {
				LOG_ERROR("[NETWORKMGR] Failed to save WiFi credentials\n");
			}
		}
		
		is_connected_ = true;
		LOG_INFO("[NETWORKMGR] WiFi credentials updated and connected successfully\n");
		printStatus();
	} else {
		is_connected_ = false;
		LOG_ERROR("[NETWORKMGR] Failed to connect with new credentials\n");
	}
	
	return success;
}

bool NetworkManager::startConfigurationPortal() {
	if (portal_started_) {
		LOG_INFO("[NETWORKMGR] WiFi configuration portal already started\n");
		return true;
	}
	
	LOG_INFO("[NETWORKMGR] Starting WiFi configuration portal...\n");
	
	// Initialize portal if not already done
	WiFiPortal::Config portal_config;
	portal_config.ap_ssid = "emfao-LedController";
	portal_config.ap_password = "12345678";
	portal_config.device_name = "LED Controller";
	portal_config.portal_timeout_ms = 0; // No timeout - stays active
	portal_config.auto_connect = true;
	
	if (!wifi_portal.initialize(portal_config, 
		[this](const std::string& ssid, const std::string& password) -> bool {
			return handlePortalConfiguration(ssid, password);
		})) {
		LOG_ERROR("[NETWORKMGR] Failed to initialize WiFi portal\n");
		return false;
	}
	
	bool success = wifi_portal.startPortal();
	if (success) {
		portal_started_ = true;
		LOG_INFO("[NETWORKMGR] Configuration portal started successfully\n");
	} else {
		LOG_ERROR("[NETWORKMGR] Failed to start configuration portal\n");
	}
	
	return success;
}

void NetworkManager::stopConfigurationPortal() {
	if (portal_started_) {
		wifi_portal.stopPortal();
		portal_started_ = false;
		LOG_INFO("[NETWORKMGR] Configuration portal stopped\n");
	}
}

bool NetworkManager::isPortalActive() const {
	return portal_started_ && wifi_portal.isActive();
}

void NetworkManager::printStatus() const {
	LOG_INFO("[NETWORKMGR] === Connection Status ===\n");
	LOG_INFO("[NETWORKMGR] Connected: %s\n", isConnected() ? "Yes" : "No");

	if (isConnected()) {
		LOG_INFO("[NETWORKMGR] SSID: %s\n", getCurrentSSID().c_str());
		LOG_INFO("[NETWORKMGR] IP Address: %s\n", getIPAddress().c_str());
		LOG_INFO("[NETWORKMGR] Signal Strength: %d dBm\n", getSignalStrength());
	}
	
	LOG_INFO("[NETWORKMGR] MAC Address: %s\n", getMACAddress().c_str());
	
	if (portal_started_) {
		LOG_INFO("[NETWORKMGR] Portal status: Active - %s\n", wifi_portal.getStatusString().c_str());
	} else {
		LOG_INFO("[NETWORKMGR] Portal status: Inactive\n");
	}
}

// === Private functions ===

bool NetworkManager::attemptConnection(int timeout_seconds) {
	WiFi.begin(credentials_.ssid.c_str(), credentials_.password.c_str());
	
	int connection_attempts = 0;
	const int max_attempts = timeout_seconds;

	LOG_INFO("[NETWORKMGR] Attempting connection...\n");

	while (WiFi.status() != WL_CONNECTED && connection_attempts < max_attempts) {
		delay(1000);
		connection_attempts++;
	}

	if (WiFi.status() == WL_CONNECTED) {
		LOG_INFO("[NETWORKMGR] Connected after %d attempts\n", connection_attempts);
	} else {
		LOG_ERROR("[NETWORKMGR] Connection failed after %d attempts\n", connection_attempts);
	}
	
	return (WiFi.status() == WL_CONNECTED);
}

void NetworkManager::logConnectionChange(bool connected) {
	if (was_connected_ != connected) {
		was_connected_ = connected;
		if (connected) {
			LOG_INFO("[NETWORKMGR] Connection state changed: CONNECTED\n");
		} else {
			LOG_INFO("[NETWORKMGR] Connection state changed: DISCONNECTED\n");
		}
	}
}

bool NetworkManager::handlePortalConfiguration(const std::string& ssid, const std::string& password) {
	LOG_INFO("[NETWORKMGR] Portal configuration received - SSID: %s\n", ssid.c_str());
	
	// Create new credentials
	Credentials new_credentials;
	new_credentials.ssid = ssid;
	new_credentials.password = password;
	
	// Update credentials (this will also test the connection)
	bool success = updateCredentials(new_credentials);
	
	if (success) {
		LOG_INFO("[NETWORKMGR] Portal configuration successful\n");
		current_attempt_count_ = 0; // Reset attempt counter
	} else {
		LOG_ERROR("[NETWORKMGR] Portal configuration failed\n");
	}
	
	return success;
}