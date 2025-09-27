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
 * @file    pca9685.h
 * @brief   Declaration of the PCA9685Module and ModuleManager class.
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <memory>
#include <vector>

#include "led.h"
#include "driver/i2c.h"


/**
 * @brief Individual PCA9685 module controller
 * 
 * This class manages a single PCA9685 PWM controller chip, including
 * its initialization, LED array, and hardware communication.
 */
class PCA9685Module {
	public:
		// === Constantes ===

		/**
		 * @var ADDR_MIN
		 * @brief First valid I2C address for PCA9685 modules.
		 *
		 * The PCA9685 supports addresses from 0x40 up to 0x7F, giving 64
		 * possible addresses. Some are reserved, so not all are usable.
		 */
		static constexpr uint8_t ADDR_MIN = 0x40;

		/**
		 * @var ADDR_MAX
		 * @brief Last valid I2C address for PCA9685 modules.
		 *
		 * Together with ::ADDR_MIN, this defines the full range of 
		 * addresses supported by the chip.
		 */
		static constexpr uint8_t ADDR_MAX = 0x7F;

		/**
		 * @var ADDR_RESERVED_ALL_CALL
		 * @brief Reserved "All Call" I2C address.
		 *
		 * The PCA9685 can be configured to respond to this address. It is used
		 * to send commands to all devices simultaneously and should not be 
		 * assigned as a normal module address.
		 */
		static constexpr uint8_t ADDR_RESERVED_ALL_CALL = 0x70;

		/**
		 * @var MODULE_MAX
		 * @brief Maximum number of PCA9685 modules supported on a single I2C bus.
		 *
		 * Although the I2C protocol allows for more, the PCA9685 is limited to 62
		 * usable addresses due to the 6 hardware address pins and reserved ranges.
		 */
		static constexpr uint8_t MODULE_MAX = 62;

		/**
		 * @var LED_MAX
		 * @brief Number of output channels per PCA9685 module.
		 *
		 * Each PCA9685 chip provides 16 independent PWM outputs, typically used
		 * to drive LEDs or servos.
		 */
		static constexpr uint8_t LED_MAX = 16;
		
	private:
		uint8_t address_;                                    ///< I2C address of the module
		bool detected_;                                      ///< Whether module was detected during scan
		bool initialized_;                                   ///< Whether module is properly initialized
		String name_;                                        ///< User-friendly name for the module
		uint8_t led_count_;                                  ///< Number of LEDs on this module
		std::unique_ptr<LED[]> leds_;                        ///< Array of LED objects
		std::unique_ptr<Adafruit_PWMServoDriver> driver_;    ///< PCA9685 driver instance

	public:
		// === Constructor and Destructor ===

		/**
		 * @brief Constructor for PCA9685Module
		 * 
		 * @param address I2C address of the module
		 * @param led_count Number of LEDs connected to this module
		 */
		PCA9685Module(uint8_t address, uint8_t led_count);
		
		/**
		 * @brief Destructor
		 */
		~PCA9685Module();
		
		// Copy constructor and assignment operator (deleted for safety)
		PCA9685Module(const PCA9685Module&) = delete;
		PCA9685Module& operator=(const PCA9685Module&) = delete;
		
		// Move constructor and assignment operator
		PCA9685Module(PCA9685Module&& other) noexcept;
		PCA9685Module& operator=(PCA9685Module&& other) noexcept;
		
		// === Getters ===

		/**
		 * @brief Check if module was detected during scan
		 * 
		 * @return true if detected, false otherwise
		 */
		bool isDetected() const { return detected_; }

		/**
		 * @brief Check if module is properly initialized
		 * 
		 * @return true if initialized, false otherwise
		 */
		bool isInitialized() const { return initialized_; }
		
		/**
		 * @brief Get module name
		 * 
		 * @return Module name as String
		 */
		const String& getName() const { return name_; }

		/**
		 * @brief Get module I2C address
		 * 
		 * @return I2C address
		 */
		uint8_t getAddress() const { return address_; }
		
		/**
		 * @brief Get number of LEDs on this module
		 * 
		 * @return LED count
		 */
		uint8_t getLedCount() const { return led_count_; }
		
		/**
		 * @brief Get LED at specified index
		 * 
		 * @param led_index LED index (0-based)
		 * @return Pointer to LED object, nullptr if invalid index
		 */
		LED* getLED(uint8_t led_index);
		
		/**
		 * @brief Get LED at specified index (const version)
		 * 
		 * @param led_index LED index (0-based)
		 * @return Pointer to LED object, nullptr if invalid index
		 */
		const LED* getLED(uint8_t led_index) const;

		// === Setters ===

		/**
		 * @brief Set module name
		 * 
		 * @param name New name for the module
		 */
		void setName(const String& name) { name_ = name; }
		
		/**
		 * @brief Set detected status
		 * 
		 * @param detected Detection status
		 */
		void setDetected(bool detected) { detected_ = detected; }

		// === Other functions ===

		/**
		 * @brief Initialize the PCA9685 module
		 * 
		 * @return true if initialization successful, false otherwise
		 */
		bool initialize();

		/**
		 * @brief Apply LED brightness to hardware
		 * 
		 * Updates the PCA9685 PWM registers for the specified LED
		 * 
		 * @param led_index LED index to update
		 * @return true if successful, false otherwise
		 */
		bool applyLedBrightness(uint8_t led_index);
		
		/**
		 * @brief Set up default LED configuration
		 * 
		 * Initializes all LEDs with default names and states
		 * 
		 * @param module_index Module index for naming convention
		 */
		void setupDefaultLeds(uint8_t module_index);
		
		/**
		 * @brief Check if this address corresponds to a PCA9685 device
		 * 
		 * @param address I2C address to check
		 * @return true if it's a PCA9685 device, false otherwise
		 */
		static bool isPCA9685Device(uint8_t address);

	private:
		// === Private functions ===

		/**
		 * @brief Generate default name for module
		 * 
		 * @return Default name based on address
		 */
		String generateDefaultName() const;
};


/**
 * @brief Manager class for all PCA9685 modules
 * 
 * This class handles scanning, initialization, and management of multiple
 * PCA9685 modules connected to the I2C bus.
 */
class ModuleManager {
	private:
		std::vector<std::unique_ptr<PCA9685Module>> modules_;   ///< Vector of managed modules

	public:
		// === Constructor and Destructor ===

		/**
		 * @brief Constructor
		 */
		ModuleManager();
		
		/**
		 * @brief Destructor
		 */
		~ModuleManager() = default;
		
		// Copy constructor and assignment operator (deleted for safety)
		ModuleManager(const ModuleManager&) = delete;
		ModuleManager& operator=(const ModuleManager&) = delete;

		// === Getters ===

		/**
		 * @brief Get total number of detected modules
		 * 
		 * @return Number of detected modules
		 */
		uint8_t getModuleCount() const { return modules_.size(); }
		
		/**
		 * @brief Get module at specified index
		 * 
		 * @param index Module index (0-based)
		 * @return Pointer to module, nullptr if invalid index
		 */
		PCA9685Module* getModule(uint8_t index);
		
		/**
		 * @brief Get module at specified index (const version)
		 * 
		 * @param index Module index (0-based)
		 * @return Pointer to module, nullptr if invalid index
		 */
		const PCA9685Module* getModule(uint8_t index) const;
		
		/**
		 * @brief Get LED from specific module and LED index
		 * 
		 * @param module_index Module index
		 * @param led_index LED index within the module
		 * @return Pointer to LED object, nullptr if invalid indices
		 */
		LED* getLED(uint8_t module_index, uint8_t led_index);
		
		/**
		 * @brief Get LED from specific module and LED index (const version)
		 * 
		 * @param module_index Module index
		 * @param led_index LED index within the module
		 * @return Pointer to LED object, nullptr if invalid indices
		 */
		const LED* getLED(uint8_t module_index, uint8_t led_index) const;

		/**
		 * @brief Get total number of LEDs across all modules
		 * 
		 * @return Total LED count
		 */
		uint16_t getTotalLedCount() const;
		
		/**
		 * @brief Get number of initialized modules
		 * 
		 * @return Count of successfully initialized modules
		 */
		uint8_t getInitializedModuleCount() const;
		
		/**
		 * @brief Get number of enabled LEDs across all modules
		 * 
		 * @return Count of enabled LEDs
		 */
		uint16_t getEnabledLedCount() const;

		// === Other functions ===
		
		/**
		 * @brief Initialize the module manager
		 * 
		 * This will scan for modules and initialize them
		 * 
		 * @return true if initialization successful, false otherwise
		 */
		bool initialize();
		
		/**
		 * @brief Apply LED brightness to hardware
		 * 
		 * @param module_index Module index
		 * @param led_index LED index within the module
		 * @return true if successful, false otherwise
		 */
		bool applyLedBrightness(uint8_t module_index, uint8_t led_index);
		
		/**
		 * @brief Print module information to Serial
		 */
		void printModuleInfo() const;

	private:
		// === Private functions ===

		/**
		 * @brief Scan I2C bus for PCA9685 modules
		 * 
		 * @return Number of modules found
		 */
		uint8_t scanModules();
		
		/**
		 * @brief Initialize all detected modules
		 * 
		 * @return Number of modules successfully initialized
		 */
		uint8_t initializeModules();
};

// Global instance
/**
 * @brief Global ModuleManager instance
 * 
 * This global instance provides access to the PCA9685 modules
 * throughout the application. It should be initialized early in
 * the setup() function and after setting up I2C bus.
 */
extern std::unique_ptr<ModuleManager> module_manager;