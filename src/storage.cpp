/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file storage.cpp
 * @brief Implementation of StorageManager class
 * 
 * This file implements the StorageManager class for ESP32.
 *
 * See storage.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include "storage.h"
#include "config.h"
#include "pca9685.h"
#include "program.h"
#include "log.h"


/// Global instance
std::unique_ptr<StorageManager> storage_manager;

/// ESP32 Preferences instance for NVS operations
Preferences StorageManager::preferences;

// === Storage Namespace Constants ===
/// @defgroup storage_namespaces Storage Namespace Definitions
/// @brief NVS namespace constants for data organization
/// @{

/// Namespace for global system configuration and metadata
const char* StorageManager::NAMESPACE_CONFIG = "config";
/// Namespace for PCA9685 module-specific configurations
const char* StorageManager::NAMESPACE_MODULES = "modules";
/// Namespace for individual LED configurations and states
const char* StorageManager::NAMESPACE_LEDS = "leds";

/// @}

bool StorageManager::initialize() {
	LOG_INFO("[STORAGEMGR] Initializing storage manager...\n");
	
	// Test if preferences can be opened
	if (!preferences.begin(NAMESPACE_CONFIG, false)) {
		LOG_ERROR("[STORAGEMGR] Failed to initialize preferences\n");
		return false;
	}
	preferences.end();
	
	LOG_INFO("[STORAGEMGR] preferences initialized successfully\n");
	return true;
}

/**
 * @internal
 * This method performs a complete wipe of all stored configuration data
 * by clearing all used namespaces. The operation is destructive and cannot
 * be undone. After calling this method, the system will use default values
 * for all settings until new configuration is saved.
 * 
 * The following namespaces are cleared:
 * - config: Global system settings
 * - modules: PCA9685 module configurations  
 * - leds: LED settings and states
 * @endinternal
 */
void StorageManager::clear_configuration() {
	LOG_INFO("[STORAGEMGR] Clearing all configuration...\n");
	
	// Clear all namespaces
	const char* namespaces[] = {NAMESPACE_CONFIG, NAMESPACE_MODULES, NAMESPACE_LEDS};
	
	for (const char* ns : namespaces) {
		if (preferences.begin(ns, false)) {
			preferences.clear();
			preferences.end();
		}
	}
	
	LOG_INFO("[STORAGEMGR] Configuration cleared");
}

/**
 * @internal
 * Serializes and saves the configuration data for a specific PCA9685 module
 * using JSON format. The saved data includes user-configurable properties
 * that should persist across system restarts.
 * 
 * Saved module properties:
 * - I2C address (for reference, though detected at runtime)
 * - User-assigned module name
 * - Detection status (for diagnostics)
 * - Initialization status (for diagnostics)
 * @endinternal
 */
bool StorageManager::save_module_config(uint8_t module_index) {
	if (!module_manager || module_index >= module_manager->getModuleCount()) {
		return false;
	}
	
	const PCA9685Module* module = module_manager->getModule(module_index);
	if (!module) {
		return false;
	}
	
	if (!preferences.begin(NAMESPACE_MODULES, false)) {
		return false;
	}
	
	String key = get_module_key(module_index);
	
	// Create JSON document for module data
	JsonDocument doc;
	doc["address"] = module->getAddress();
	doc["name"] = module->getName();
	doc["detected"] = module->isDetected();
	doc["initialized"] = module->isInitialized();
	
	// Serialize to string
	String json_string;
	serializeJson(doc, json_string);
	
	bool success = preferences.putString(key.c_str(), json_string);
	preferences.end();
	
	if (success) {
		LOG_INFO("[STORAGEMGR] Module %d configuration saved\n", module_index);
	}
	
	return success;
}

/**
 * @internal
 * Loads and applies saved configuration data for a specific PCA9685 module.
 * Only user-configurable properties are applied, while hardware-dependent
 * values (like I2C address and detection status) are determined at runtime.
 * 
 * Loaded module properties:
 * - User-assigned module name (applied if present)
 * - Other user preferences (if any)
 * 
 * Properties NOT loaded (determined at runtime):
 * - I2C address (detected during hardware scan)
 * - Detection status (result of hardware probe)
 * - Initialization status (result of hardware setup)
 * @endinternal
 */
bool StorageManager::load_module_config(uint8_t module_index) {
	// Return if module index is not valid
	if (!module_manager || module_index >= module_manager->getModuleCount()) {
		return false;
	}
	
	// Get module
	PCA9685Module* module = module_manager->getModule(module_index);
	if (!module) {
		return false;
	}
	
	// Get module config
	if (!preferences.begin(NAMESPACE_MODULES, true)) {
		return false;
	}
	String key = get_module_key(module_index);
	String json_string = preferences.getString(key.c_str(), "");
	preferences.end();
	
	if (json_string.isEmpty()) {
		return false;
	}
	
	// Parse JSON
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, json_string);
	
	if (error) {
		LOG_ERROR("[STORAGEMGR] Failed to parse module config: %s\n", error.c_str());
		return false;
	}
	
	// Load module data (only name, as address and status are detected)
	if (doc["name"].is<String>()) {
		module->setName(doc["name"].as<String>());
	}
	
	LOG_INFO("[STORAGEMGR] Module %d configuration loaded\n", module_index);
	
	return true;
}

/**
 * @internal
 * Serializes and saves all configuration data for a specific LED using
 * JSON format. This includes all user-configurable settings and current
 * state that should be restored after system restart.
 * 
 * Saved LED properties:
 * - User-assigned name
 * - Enable/disable state
 * - Current brightness value (0-4095)
 * - Current program type assignment
 * @endinternal
 */
bool StorageManager::save_led_config(uint8_t module_index, uint8_t led_index) {
	if (!module_manager || module_index >= module_manager->getModuleCount()) {
		return false;
	}
	
	const PCA9685Module* module = module_manager->getModule(module_index);
	if (!module || led_index >= module->getLedCount()) {
		return false;
	}
	
	const LED* led = module->getLED(led_index);
	if (!led) {
		return false;
	}
	
	
	String key = get_led_key(module_index, led_index);
	
	// Create JSON document for LED data
	JsonDocument doc;
	doc["name"] = led->getName();
	doc["enabled"] = led->isEnabled();
	doc["brightness"] = led->getBrightness();
	doc["program_type"] = led->getProgramType();
	
	// Serialize to string
	String json_string;
	serializeJson(doc, json_string);

	LOG_DEBUG("[STORAGEMGR] Saving LED %d_%d - Raw JSON: %s\n", module_index, led_index, json_string.c_str());
	
	if (!preferences.begin(NAMESPACE_LEDS, false)) {
		return false;
	}
	bool success = preferences.putString(key.c_str(), json_string);
	preferences.end();

	if (success) {
		LOG_INFO("[STORAGEMGR] LED %d_%d configuration saved\n", module_index, led_index);
	} else {
		LOG_ERROR("[STORAGEMGR] Saving LED %d_%d configuration failed", module_index, led_index);
	}
	
	return success;
}

/**
 * @internal
 * Loads and applies saved configuration data for a specific LED. The
 * configuration is immediately applied to both the LED object and the
 * physical hardware.
 * 
 * Loaded LED properties:
 * - User-assigned name (if saved)
 * - Enable/disable state (if saved)
 * - Brightness value (if saved, applied to hardware)
 * - Program type assignment (if saved)
 * @endinternal
 */
bool StorageManager::load_led_config(uint8_t module_index, uint8_t led_index) {
	// Return if module index is invalid
	if (!module_manager || module_index >= module_manager->getModuleCount()) {
		return false;
	}
	
	// Return if led index is invalid
	const PCA9685Module* module = module_manager->getModule(module_index);
	if (!module || led_index >= module->getLedCount()) {
		return false;
	}
	
	// Get LED
	LED* led = module_manager->getLED(module_index, led_index);
	if (!led) {
		return false;
	}
	
	// Get LED config
	if (!preferences.begin(NAMESPACE_LEDS, true)) {
		return false;
	}
	String key = get_led_key(module_index, led_index);
	String json_string = preferences.getString(key.c_str(), "");
	preferences.end();
	
	if (json_string.isEmpty()) {
		LOG_WARNING("[STORAGEMGR] No config found for LED %d_%d\n", module_index, led_index);
		return false;
	}
	
	// DEBUG Print raw JSON data
	LOG_DEBUG("[STORAGEMGR] Loading LED %d_%d - Raw JSON: %s\n", module_index, led_index, json_string.c_str());
	
	// Parse JSON
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, json_string);
	
	if (error) {
		LOG_ERROR("[STORAGEMGR] JSON parse error for LED %d_%d: %s\n", module_index, led_index, error.c_str());
		return false;
	}
	
	// Load LED data
	if (doc["name"].is<String>()) {
		led->setName(doc["name"].as<String>());
	}
	if (doc["enabled"].is<bool>()) {
		led->setEnabled(doc["enabled"]);
	}
	if (doc["brightness"].is<uint16_t>()) {
		led->setBrightness(doc["brightness"]);
	}
	if (doc["program_type"].is<int>()) {
		led->setProgram(doc["program_type"], nullptr);
		program_manager->assign_program(module_index, led_index, doc["program_type"]);
	}
	
	// Apply the loaded brightness
	module_manager->applyLedBrightness(module_index, led_index);

	return true;
}

/**
 * @internal
 * Creates a standardized storage key for PCA9685 module configuration data.
 * The key format ensures uniqueness across all modules and follows the
 * pattern "mod_{module_index}".
 * @endinternal
 */
String StorageManager::get_module_key(uint8_t module_index) {
	return "mod_" + String(module_index);
}

/**
 * @internal
 * Creates a standardized storage key for individual LED configuration data.
 * The key format ensures uniqueness across all LEDs in all modules and
 * follows the pattern "led_{module_index}_{led_index}".
 * @endinternal
 */
String StorageManager::get_led_key(uint8_t module_index, uint8_t led_index) {
	return "led_" + String(module_index) + "_" + String(led_index);
}

bool StorageManager::save_configuration() {
	LOG_INFO("[STORAGEMGR] Saving complete configuration...\n");
	
	if (!module_manager) {
		return false;
	}
	
	bool success = true;
	
	// Save modules configuration
	for (uint8_t i = 0; i < module_manager->getModuleCount(); i++) {
		if (!save_module_config(i)) {
			success = false;
		}
			
		// Save LEDs for this module
		const PCA9685Module* module = module_manager->getModule(i);
		if (module) {
			for (uint8_t j = 0; j < module->getLedCount(); j++) {
				if (!save_led_config(i, j)) {
					success = false;
				}
			}
		}
	}
	
	// Save global configuration
	if (preferences.begin(NAMESPACE_CONFIG, false)) {
		preferences.putUChar("module_count", module_manager->getModuleCount());
		preferences.putULong("last_save", millis());
		preferences.end();
	} else {
		success = false;
	}
	
	if (success) {
		LOG_INFO("[STORAGEMGR] Configuration saved successfully\n");
	} else {
		LOG_ERROR("[STORAGEMGR] Some configuration items failed to save\n");
	}
	
	return success;
}

bool StorageManager::load_configuration() {
	LOG_INFO("[STORAGEMGR] Loading configuration...\n");
	
	if (!module_manager) {
		return false;
	}
	
	// Load global configuration
	if (!preferences.begin(NAMESPACE_CONFIG, true)) {
		LOG_ERROR("[STORAGEMGR] No configuration found\n");
		return false;
	}
	
	uint8_t saved_module_count = preferences.getUChar("module_count", 0);
	unsigned long last_save = preferences.getULong("last_save", 0);
	preferences.end();
	
	if (saved_module_count == 0) {
		LOG_INFO("[STORAGEMGR] No modules configuration found\n");
		return false;
	}
	
	LOG_INFO("[STORAGEMGR] Found configuration with %d modules\n", saved_module_count);
	
	// Load modules and LEDs configuration
	bool success = true;
	uint8_t loaded_modules = 0;
	
	for (uint8_t i = 0; i < module_manager->getModuleCount() && i < saved_module_count; i++) {
		if (load_module_config(i)) {
			loaded_modules++;
			
			// Load LEDs for this module
			const PCA9685Module* module = module_manager->getModule(i);
			if (module) {
				for (uint8_t j = 0; j < module->getLedCount(); j++) {
					load_led_config(i, j);
				}
			}
		}
	}
	
	LOG_INFO("[STORAGEMGR] Loaded configuration for %d modules\n", loaded_modules);
	
	return loaded_modules > 0;
}

// === WiFi Configuration Management ===

bool StorageManager::save_wifi_credentials(const std::string& ssid, const std::string& password) {
	LOG_INFO("[STORAGEMGR] Saving WiFi credentials...\n");
	
	if (ssid.empty() || password.empty()) {
		LOG_ERROR("[STORAGEMGR] Invalid WiFi credentials\n");
		return false;
	}
	
	if (!preferences.begin(NAMESPACE_CONFIG, false)) {
		LOG_ERROR("[STORAGEMGR] Failed to open config namespace\n");
		return false;
	}
	
	bool success = true;
	if (!preferences.putString("wifi_ssid", ssid.c_str())) {
		success = false;
	}
	if (!preferences.putString("wifi_password", password.c_str())) {
		success = false;
	}
	
	preferences.end();
	
	if (success) {
		LOG_INFO("[STORAGEMGR] WiFi credentials saved successfully\n");
	} else {
		LOG_ERROR("[STORAGEMGR] Failed to save WiFi credentials\n");
	}
	
	return success;
}

bool StorageManager::load_wifi_credentials(std::string& ssid, std::string& password) {
	LOG_INFO("[STORAGEMGR] Loading WiFi credentials...\n");
	
	if (!preferences.begin(NAMESPACE_CONFIG, true)) {
		LOG_INFO("[STORAGEMGR] No config namespace found\n");
		return false;
	}
	
	String wifi_ssid = preferences.getString("wifi_ssid", "");
	String wifi_password = preferences.getString("wifi_password", "");
	preferences.end();
	
	if (wifi_ssid.isEmpty() || wifi_password.isEmpty()) {
		LOG_INFO("[STORAGEMGR] No WiFi credentials found in storage\n");
		return false;
	}
	
	ssid = wifi_ssid.c_str();
	password = wifi_password.c_str();
	
	LOG_INFO("[STORAGEMGR] WiFi credentials loaded for SSID: %s\n", ssid.c_str());
	return true;
}

bool StorageManager::has_wifi_credentials() {
	if (!preferences.begin(NAMESPACE_CONFIG, true)) {
		return false;
	}
	
	bool has_ssid = preferences.isKey("wifi_ssid");
	bool has_password = preferences.isKey("wifi_password");
	preferences.end();
	
	return has_ssid && has_password;
}

bool StorageManager::clear_wifi_credentials() {
	LOG_INFO("[STORAGEMGR] Clearing WiFi credentials...\n");
	
	if (!preferences.begin(NAMESPACE_CONFIG, false)) {
		return false;
	}
	
	preferences.remove("wifi_ssid");
	preferences.remove("wifi_password");
	preferences.end();
	
	LOG_INFO("[STORAGEMGR] WiFi credentials cleared\n");
	return true;
}