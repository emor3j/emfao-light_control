/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Implementation of OTAManager.
 *
 * See ota.h for API documentation.
 *
 * @file ota.cpp
 * @brief Implementation of OTA manager class
 * 
 * This file implements the OTAManager class for ESP32 system.
 *
 * See ota.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include <WiFi.h>
#include <ESPmDNS.h>

#include "ota.h"
#include "network.h"
#include "log.h"



// Global instance
std::unique_ptr<OTAManager> ota_manager;

// === Constructor and Destructor ===

// Default constructor
OTAManager::OTAManager() :
	status_(Status::IDLE),
	current_progress_(0),
	total_size_(0),
	current_size_(0),
	start_time_(0),
	last_progress_time_(0),
	initialized_(false),
	active_(false) {}

// Destructor
OTAManager::~OTAManager() {
	if (active_) {
		stop();
	}
}

// === Getters ===

std::string OTAManager::getStatusString() const {
	switch (status_) {
		case Status::IDLE:        return "Idle";
		case Status::INITIALIZING: return "Initializing";
		case Status::READY:       return "Ready";
		case Status::UPDATING:    return "Updating";
		case Status::SUCCESS:     return "Success";
		case Status::FAILED:      return "Failed";
		case Status::REBOOTING:   return "Rebooting";
		default:                  return "Unknown";
	}
}

bool OTAManager::isUpdating() const {
	return status_ == Status::UPDATING;
}

// === Other functions ===

bool OTAManager::initialize(const Config& config) {
	LOG_INFO("[OTAMGR] Initializing OTA manager...\n");
	
	// Check WiFi connection
	if (!WiFi.isConnected()) {
		LOG_ERROR("[OTAMGR] Error: WiFi not connected\n");
		updateStatus(Status::FAILED, "WiFi not connected");
		return false;
	}

	if (!isWiFiStable()) {
		LOG_WARNING("[OTAMGR] WiFi connection may be unstable\n");
	}
	
	config_ = config;
	
	LOG_INFO("[OTAMGR] Hostname: %s\n", config_.hostname.c_str());
	LOG_INFO("[OTAMGR] Port: %d\n", config_.port);
	LOG_INFO("[OTAMGR] Authentication: %s\n", config_.password.empty() ? "Disabled" : "Enabled");
	LOG_INFO("[OTAMGR] Auto-reboot: %s\n", config_.auto_reboot ? "Enabled" : "Disabled");
	LOG_INFO("[OTAMGR] Timeout: %ds\n", config_.timeout_ms / 1000);
	
	// Set OTA configuration
	ArduinoOTA.setHostname(config_.hostname.c_str());
	ArduinoOTA.setPort(config_.port);
	
	if (!config_.password.empty()) {
		ArduinoOTA.setPassword(config_.password.c_str());
		LOG_INFO("[OTAMGR] Password protection enabled\n");
	}
	
	// Setup event handlers
	setupEventHandlers();
	
	// Enable mDNS if requested
	if (config_.enable_mdns) {
		if (MDNS.begin(config_.hostname.c_str())) {
			LOG_INFO("[OTAMGR] mDNS responder started\n");
			MDNS.addService("arduino", "tcp", config_.port);
		} else {
			LOG_ERROR("[OTAMGR] Warning: mDNS initialization failed\n");
		}
	}

	// Enable mDNS if requested
	if (config_.enable_mdns) {
		// Check if mDNS is already initialized
		if (!MDNS.begin(config_.hostname.c_str())) {
			LOG_ERROR("[OTAMGR] Warning: mDNS initialization failed\n");
			// Try to end any existing mDNS and retry
			MDNS.end();
			delay(100);
			if (MDNS.begin(config_.hostname.c_str())) {
				LOG_INFO("[OTAMGR] mDNS responder started on retry\n");
				MDNS.addService("arduino", "tcp", config_.port);
			} else {
				LOG_ERROR("[OTAMGR] mDNS still failed, continuing without it\n");
			}
		} else {
			LOG_INFO("[OTAMGR] mDNS responder started\n");
			MDNS.addService("arduino", "tcp", config_.port);
		}
	}
	
	initialized_ = true;
	updateStatus(Status::INITIALIZING, "OTA manager initialized");
	
	LOG_INFO("[OTAMGR] OTA manager initialized successfully\n");
	return true;
}

bool OTAManager::start() {
	if (!initialized_) {
		LOG_ERROR("[OTAMGR] Error: OTA manager not initialized\n");
		return false;
	}
	
	if (active_) {
		LOG_INFO("[OTAMGR] OTA service already running\n");
		return true;
	}
	
	LOG_INFO("[OTAMGR] Starting OTA service...\n");
	
	// Final WiFi stability check
	if (!isWiFiStable()) {
		LOG_ERROR("[OTAMGR] Error: WiFi connection unstable, cannot start OTA\n");
		updateStatus(Status::FAILED, "WiFi connection unstable");
		return false;
	}
	
	ArduinoOTA.begin();
	active_ = true;
	
	updateStatus(Status::READY, "OTA service ready");
	
	LOG_INFO("[OTAMGR] OTA service started successfully\n");
	LOG_INFO("[OTAMGR] Device discoverable as: %s\n", config_.hostname.c_str());
	LOG_INFO("[OTAMGR] IP Address: %s\n", WiFi.localIP().toString().c_str());
	LOG_INFO("[OTAMGR] Signal Strength: %d dBm\n", WiFi.RSSI());

	return true;
}

void OTAManager::stop() {
	if (!active_) {
		return;
	}
	
	LOG_INFO("[OTAMGR] Stopping OTA service...\n");
	
	ArduinoOTA.end();
	active_ = false;
	
	updateStatus(Status::IDLE, "OTA service stopped");
	LOG_INFO("[OTAMGR] OTA service stopped\n");
}

void OTAManager::handle() {
	if (!active_) {
		return;
	}
	
	// Check WiFi connection stability during operation
	static unsigned long last_wifi_check = 0;
	unsigned long current_time = millis();
	
	if (current_time - last_wifi_check >= 5000) { // Check every 5 seconds
		last_wifi_check = current_time;
		
		if (!isWiFiStable()) {
			LOG_WARNING("[OTAMGR] WiFi connection unstable during OTA operation\n");
			if (status_ == Status::UPDATING) {
				LOG_ERROR("[OTAMGR] Critical: WiFi unstable during update - this may cause corruption\n");
			}
		}
	}
	
	// Handle timeout during updates
	if (status_ == Status::UPDATING) {
		unsigned long elapsed = current_time - start_time_;
		if (elapsed > config_.timeout_ms) {
			LOG_ERROR("[OTAMGR] Error: Update timeout after %ds\n", elapsed / 1000);
			updateStatus(Status::FAILED, "Update timeout");
		}
		
		// Check for stalled progress
		if (current_time - last_progress_time_ > 10000) { // 10 seconds without progress
			LOG_WARNING("[OTAMGR] Update progress stalled");
		}
	}
	
	ArduinoOTA.handle();
}

// === Private methods ===

void OTAManager::setupEventHandlers() {
	LOG_INFO("[OTAMGR] Setting up event handlers...\n");
	
	ArduinoOTA.onStart([this]() {
		handleStart();
	});
	
	ArduinoOTA.onEnd([this]() {
		handleEnd();
	});
	
	ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
		handleProgress(progress, total);
	});
	
	ArduinoOTA.onError([this](ota_error_t error) {
		handleError(error);
	});
	
	LOG_INFO("[OTAMGR] Event handlers configured\n");
}

void OTAManager::handleStart() {
	String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
	
	LOG_INFO("[OTAMGR] OTA UPDATE STARTED\n");
	LOG_INFO("[OTAMGR] Update type: %s\n", type.c_str());
	LOG_INFO("[OTAMGR] Free heap before update: %d bytes\n", ESP.getFreeHeap());
	LOG_INFO("[OTAMGR] WiFi signal: %d dBm\n", WiFi.RSSI());
	
	start_time_ = millis();
	last_progress_time_ = start_time_;
	current_progress_ = 0;
	total_size_ = 0;
	current_size_ = 0;
	
	char buffer[100];
	snprintf(buffer, sizeof(buffer), "OTA update started (%s)", type.c_str());
	updateStatus(Status::UPDATING, std::string(buffer));
	
	// NOTE: All Serial.end() calls are avoided as they can cause issues
	// The system will handle serial communication properly during OTA
}

void OTAManager::handleEnd() {
	unsigned long elapsed = millis() - start_time_;

	LOG_INFO("[OTAMGR] OTA UPDATE COMPLETED\n");
	LOG_INFO("[OTAMGR] Total time: %ds\n", elapsed / 1000);

	if (total_size_ > 0) {
		LOG_INFO("[OTAMGR] Total size: %d bytes\n", total_size_);
		LOG_INFO("[OTAMGR] Average speed: %d bytes/sec\n", (total_size_ * 1000) / elapsed);
	}
	
	LOG_INFO("[OTAMGR] Free heap after update: %d bytes\n", ESP.getFreeHeap());
	
	updateStatus(Status::SUCCESS, "OTA update completed successfully");
	
	if (config_.auto_reboot) {
		LOG_INFO("[OTAMGR] Rebooting in 2 seconds...\n");
		updateStatus(Status::REBOOTING, "System rebooting");
		delay(2000);
		ESP.restart();
	} else {
		LOG_INFO("[OTAMGR] Auto-reboot disabled, manual restart required\n");
	}
}

void OTAManager::handleProgress(unsigned int progress, unsigned int total) {
	current_size_ = progress;
	total_size_ = total;
	last_progress_time_ = millis();
	
	uint8_t percentage = (progress * 100) / total;
	current_progress_ = percentage;
	
	// Detailed progress every 10%
	if (percentage % 10 == 0 || percentage == 1) {
		unsigned long elapsed = millis() - start_time_;
		unsigned long speed = elapsed > 0 ? (progress * 1000) / elapsed : 0;
		
		LOG_INFO("[OTAMGR] Progress: %u%% (%u/%u bytes) | Speed: %lu B/s | Elapsed: %lu ms | Heap: %u bytes\n",
					 percentage, progress, total, speed, elapsed, ESP.getFreeHeap());
		
		// WiFi quality check during transfer
		int32_t rssi = WiFi.RSSI();
		if (rssi < -80) {
			LOG_WARNING("[OTAMGR] Weak WiFi signal: %d dBm\n", rssi);
		}
	}
	
	// Call user callback if set
	if (progress_callback_) {
		progress_callback_(percentage, total, progress);
	}
}

void OTAManager::handleError(ota_error_t error) {
	std::string error_msg = errorToString(error);
	
	LOG_ERROR("[OTAMGR] OTA UPDATE FAILED\n");
	LOG_ERROR("[OTAMGR] Error: %s\n", error_msg.c_str());
	
	// Additional diagnostic information
	LOG_INFO("[OTAMGR] WiFi Status: %d\n", WiFi.status());
	LOG_INFO("[OTAMGR] WiFi RSSI: %d dBm\n", WiFi.RSSI());
	LOG_INFO("[OTAMGR] Free Heap: %d bytes\n", ESP.getFreeHeap());
	
	if (total_size_ > 0) {
		LOG_INFO("[OTAMGR] Progress at failure: %d% (%d/%d bytes)\n",
			current_progress_, current_size_, total_size_);
	}
	
	unsigned long elapsed = millis() - start_time_;
	LOG_INFO("[OTAMGR] Time elapsed: %dms\n", elapsed);
	
	last_error_ = error_msg;
	updateStatus(Status::FAILED, std::string("OTA update failed: ") + error_msg);
}

std::string OTAManager::errorToString(ota_error_t error) const {
	switch (error) {
		case OTA_AUTH_ERROR:
			return "Authentication failed - check password";
		case OTA_BEGIN_ERROR:
			return "Begin failed - insufficient space or invalid firmware";
		case OTA_CONNECT_ERROR:
			return "Connect failed - network connectivity issue";
		case OTA_RECEIVE_ERROR:
			return "Receive failed - data transfer error";
		case OTA_END_ERROR:
			return "End failed - firmware verification error";
		default:
			return "Unknown error (" + std::to_string(error) + ")";
	}
}

void OTAManager::updateStatus(Status new_status, const std::string& message) {
	if (status_ != new_status) {
		status_ = new_status;
		
		if (status_callback_) {
			status_callback_(status_, message);
		}
	}
}

bool OTAManager::isWiFiStable() const {
	if (!network_manager->isConnected()) {
		return false;
	}

	// Check if we have a valid IP
	IPAddress ip;
	ip.fromString(network_manager->getIPAddress().c_str());
	if (ip == IPAddress(0, 0, 0, 0)) {
	   LOG_WARNING("[OTAMGR] No valid IP address\n");
	   return false;
	}
	
	// Return false when signal is very weak
	int32_t rssi = WiFi.RSSI();
	if (rssi < -75) {
		LOG_WARNING("[OTAMGR] WiFi signal very weak: %d dBm\n", rssi);
		return false;
	}
	
	
	
	return true;
}