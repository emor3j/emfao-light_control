/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of emfao-light_control.
 *
 * emfao-light_control is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emfao-light_control is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with emfao-light_control.  If not, see <https://www.gnu.org/licenses/>.
 *
 * @file  storage.h
 * @brief Persistent storage management for LED controller configuration
 * 
 * This header defines the StorageManager class that provides persistent storage
 * capabilities for the LED controller system. It manages configuration data for
 * modules, LEDs, and programs using the ESP32's NVS (Non-Volatile Storage) system
 * through the Arduino Preferences library.
 * 
 * The storage system uses multiple namespaces to organize different types of data:
 * - config: Global system configuration
 * - modules: PCA9685 module-specific settings
 * - leds: Individual LED configurations
 * - programs: LED program assignments and parameters
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <memory>


/**
 * @class StorageManager
 * @brief Static class for managing persistent configuration storage
 * 
 * The StorageManager provides a comprehensive interface for saving and loading
 * all system configuration data to/from ESP32's non-volatile storage. It uses
 * JSON serialization for complex data structures and the Preferences library
 * for efficient key-value storage.
 * 
 * The class automatically handles namespace management and provides atomic
 * operations for configuration persistence.
 * 
 * @note The storage system is designed to be fault-tolerant and will gracefully
 *       handle missing or corrupted configuration data by returning appropriate
 *       default values.
 * @note All methods are static as there should be only one storage manager
 *       instance in the system.
 */
class StorageManager {
	private:		
		/// ESP32 Preferences instance for NVS access
		static Preferences preferences;
		
		/// @defgroup storage_namespaces Storage Namespaces
		/// @brief NVS namespace constants for organizing different data types
		/// @{
		
		/// Namespace for global system configuration
		static const char* NAMESPACE_CONFIG;
		/// Namespace for PCA9685 module configurations
		static const char* NAMESPACE_MODULES;
		/// Namespace for individual LED configurations
		static const char* NAMESPACE_LEDS;
		
		/// @}
		
		// === Key Generation Methods ===
		
		/**
		 * @brief Generate storage key for module configuration
		 * 
		 * Creates a unique storage key for a PCA9685 module configuration.
		 * The key format is "mod_{module_index}".
		 * 
		 * @param module_index Index of the PCA9685 module (0-based)
		 * 
		 * @return String containing the unique storage key for the module
		 */
		static String get_module_key(uint8_t module_index);
		
		/**
		 * @brief Generate storage key for LED configuration
		 * 
		 * Creates a unique storage key for an individual LED configuration.
		 * The key format is "led_{module_index}_{led_index}".
		 * 
		 * @param module_index Index of the PCA9685 module (0-based)
		 * @param led_index Index of the LED within the module (0-based)
		 * 
		 * @return String containing the unique storage key for the LED
		 */
		static String get_led_key(uint8_t module_index, uint8_t led_index);

	public:
		// === Initialization and Global Operations ===
		
		/**
		 * @brief Initialize the storage manager
		 * 
		 * Prepares the storage system for operation by testing NVS access
		 * and validating that all required namespaces can be created.
		 * Must be called once during system initialization before any
		 * other storage operations.
		 * 
		 * @return true if initialization successful and storage is ready,
		 *         false if NVS system is unavailable or corrupted
		 * 
		 * @note This method performs a test write/read operation to verify
		 *       that the storage system is functional
		 */
		static bool initialize();
		
		/**
		 * @brief Save complete system configuration
		 * 
		 * Performs a comprehensive save of all system configuration including:
		 * - All PCA9685 module configurations
		 * - All LED settings and states
		 * - Global system parameters
		 * - Timestamp of last save operation
		 * 
		 * This is a high-level operation that calls all individual save methods
		 * and provides a single point for complete system backup.
		 * 
		 * @return true if all configuration data was saved successfully,
		 *         false if any part of the save operation failed
		 * 
		 * @note Even if this method returns false, some configuration may
		 *       have been saved successfully. Individual save methods can
		 *       be called to retry specific parts.
		 */
		static bool save_configuration();
		
		/**
		 * @brief Load complete system configuration
		 * 
		 * Performs a comprehensive load of all saved system configuration
		 * and applies it to the current system state. This includes:
		 * - PCA9685 module configurations
		 * - LED settings and brightness values
		 * - Global system parameters
		 * 
		 * Hardware settings (brightness values) are automatically applied
		 * to the physical LEDs during the load process.
		 * 
		 * @return true if configuration was loaded and applied successfully,
		 *         false if no saved configuration exists or load failed
		 * 
		 * @note Partial loads are supported - individual components that
		 *       fail to load will use default values while others are
		 *       restored from storage
		 */
		static bool load_configuration();
		
		/**
		 * @brief Clear all stored configuration data
		 * 
		 * Removes all configuration data from persistent storage across
		 * all namespaces. This is a destructive operation that cannot be
		 * undone and will result in the system using default values on
		 * next startup.
		 * 
		 * @warning This operation is irreversible and will delete all
		 *          saved settings, LED configurations, and program
		 *          assignments
		 * 
		 * @note The system will continue to operate normally after this
		 *       operation using default values, but all customizations
		 *       will be lost
		 */
		static void clear_configuration();
		
		// === Module Configuration Management ===
		
		/**
		 * @brief Save configuration for a specific PCA9685 module
		 * 
		 * Saves all configuration data for the specified module including:
		 * - Module name and description
		 * - I2C address
		 * - Detection and initialization status
		 * - Module-specific parameters
		 * 
		 * @param module_index Index of the module to save (0-based)
		 * 
		 * @return true if module configuration saved successfully,
		 *         false if module doesn't exist or save failed
		 * 
		 * @note Only user-configurable properties are saved. Hardware
		 *       detection results are determined at runtime
		 */
		static bool save_module_config(uint8_t module_index);
		
		/**
		 * @brief Load configuration for a specific PCA9685 module
		 * 
		 * Loads and applies saved configuration data for the specified module.
		 * Only applies settings that don't conflict with hardware detection
		 * results (e.g., names and user preferences, not I2C addresses).
		 * 
		 * @param module_index Index of the module to load (0-based)
		 * 
		 * @return true if module configuration loaded successfully,
		 *         false if no saved config exists, module doesn't exist,
		 *         or load failed
		 */
		static bool load_module_config(uint8_t module_index);
		
		// === LED Configuration Management ===
		
		/**
		 * @brief Save configuration for a specific LED
		 * 
		 * Saves all configuration data for the specified LED including:
		 * - User-assigned name
		 * - Current brightness setting
		 * - Enable/disable state
		 * - Current program assignment
		 * 
		 * @param module_index Index of the PCA9685 module (0-based)
		 * @param led_index Index of the LED within the module (0-based)
		 * 
		 * @return true if LED configuration saved successfully,
		 *         false if LED doesn't exist or save failed
		 */
		static bool save_led_config(uint8_t module_index, uint8_t led_index);
		
		/**
		 * @brief Load configuration for a specific LED
		 * 
		 * Loads and applies saved configuration data for the specified LED.
		 * The brightness setting is automatically applied to the physical
		 * LED hardware during the load process.
		 * 
		 * @param module_index Index of the PCA9685 module (0-based)
		 * @param led_index Index of the LED within the module (0-based)
		 * 
		 * @return true if LED configuration loaded and applied successfully,
		 *         false if no saved config exists, LED doesn't exist,
		 *         or load failed
		 * 
		 * @note The physical LED brightness is updated immediately after
		 *       loading the configuration
		 */
		static bool load_led_config(uint8_t module_index, uint8_t led_index);

		// === WiFi Configuration Management ===

		/**
		 * @brief Save WiFi credentials to storage
		 * 
		 * @param ssid WiFi network SSID
		 * @param password WiFi network password
		 * @return true if credentials saved successfully
		 */
		static bool save_wifi_credentials(const std::string& ssid, const std::string& password);

		/**
		 * @brief Load WiFi credentials from storage
		 * 
		 * @param ssid Reference to store loaded SSID
		 * @param password Reference to store loaded password
		 * @return true if credentials loaded successfully
		 */
		static bool load_wifi_credentials(std::string& ssid, std::string& password);

		/**
		 * @brief Check if WiFi credentials exist in storage
		 * 
		 * @return true if credentials are stored
		 */
		static bool has_wifi_credentials();

		/**
		 * @brief Clear WiFi credentials from storage
		 * 
		 * @return true if credentials cleared successfully
		 */
		static bool clear_wifi_credentials();
};

/**
 * @brief Global StorageManager instance
 * 
 * This global instance provides access to the storage system
 * throughout the application. It should be initialized before something
 * need to load some variables.
 */
extern std::unique_ptr<StorageManager> storage_manager;