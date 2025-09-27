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
 * @file    config.h
 * @brief   Declaration of the Config class.
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <Arduino.h>


/**
 * @brief Configuration class for ESP32 LED controller system
 * 
 * This class encapsulates all configuration parameters for the ESP32-based
 * LED controller that manages PWM LEDs through PCA9685 modules via I2C bus.
 */
class Config {
	private:
		uint8_t i2c_pin_sda_;        ///< I2C SDA pin number
		uint8_t i2c_pin_scl_;        ///< I2C SCL pin number
		uint8_t pca9685_addr_min_;   ///< Minimum PCA9685 I2C address
		uint8_t pca9685_addr_max_;   ///< Maximum PCA9685 I2C address
		uint8_t pca9685_module_max_; ///< Maximum number of PCA9685 modules supported
		uint8_t pca9685_led_max_;    ///< Maximum number of LEDs per PCA9685 module
		size_t led_name_max_;        ///< Maximum length for LED names

		/**
		 * @brief Check if a GPIO pin number is valid for ESP32
		 * @param pin GPIO pin number to check
		 * @return true if pin is a valid GPIO pin
		 */
		static bool isValidGpioPin(uint8_t pin);

	public:
		// === Constructor and Destructor ===
		
		/**
		 * @brief Default constructor with standard configuration values
		 * 
		 * Initializes the configuration with commonly used default values:
		 * - I2C pins: SDA=21, SCL=22 (standard ESP32 pins)
		 * - PCA9685 address range: 0x40-0x7F (standard I2C address range)
		 * - Maximum 16 modules with 16 LEDs each
		 * - LED name maximum length: 64 characters
		 */
		Config();

		/**
		 * @brief Parametric constructor for custom configuration
		 * 
		 * @param sda_pin I2C SDA pin number
		 * @param scl_pin I2C SCL pin number
		 * @param addr_min Minimum PCA9685 I2C address
		 * @param addr_max Maximum PCA9685 I2C address
		 * @param module_max Maximum number of PCA9685 modules
		 * @param led_max Maximum number of LEDs per module
		 * @param name_max Maximum length for LED names
		 */
		Config(uint8_t sda_pin, uint8_t scl_pin, uint8_t addr_min, uint8_t addr_max,
			uint8_t module_max, uint8_t led_max, size_t name_max);

		// === Getters ===

		/**
		 * @brief Get I2C SDA pin number
		 * @return SDA pin number
		 */
		uint8_t getI2cSdaPin() const { return i2c_pin_sda_; }

		/**
		 * @brief Get I2C SCL pin number
		 * @return SCL pin number
		 */
		uint8_t getI2cSclPin() const { return i2c_pin_scl_; }

		/**
		 * @brief Get minimum PCA9685 I2C address
		 * @return Minimum I2C address
		 */
		uint8_t getPca9685AddrMin() const { return pca9685_addr_min_; }

		/**
		 * @brief Get maximum PCA9685 I2C address
		 * @return Maximum I2C address
		 */
		uint8_t getPca9685AddrMax() const { return pca9685_addr_max_; }

		/**
		 * @brief Get maximum number of PCA9685 modules supported
		 * @return Maximum module count
		 */
		uint8_t getPca9685ModuleMax() const { return pca9685_module_max_; }

		/**
		 * @brief Get maximum number of LEDs per PCA9685 module
		 * @return Maximum LED count per module
		 */
		uint8_t getPca9685LedMax() const { return pca9685_led_max_; }

		/**
		 * @brief Get maximum length for LED names
		 * @return Maximum name length in characters
		 */
		size_t getLedNameMax() const { return led_name_max_; }

		// === Setters with validation ===

		/**
		 * @brief Set I2C SDA pin number
		 * @param pin SDA pin number (must be valid GPIO pin)
		 * @return true if pin is valid and set successfully
		 */
		bool setI2cSdaPin(uint8_t pin);

		/**
		 * @brief Set I2C SCL pin number
		 * @param pin SCL pin number (must be valid GPIO pin)
		 * @return true if pin is valid and set successfully
		 */
		bool setI2cSclPin(uint8_t pin);

		/**
		 * @brief Set PCA9685 address range
		 * @param min_addr Minimum I2C address
		 * @param max_addr Maximum I2C address
		 * @return true if range is valid and set successfully
		 */
		bool setPca9685AddressRange(uint8_t min_addr, uint8_t max_addr);

		/**
		 * @brief Set maximum number of PCA9685 modules
		 * @param max_modules Maximum module count (1-62 for PCA9685)
		 * @return true if value is valid and set successfully
		 */
		bool setPca9685ModuleMax(uint8_t max_modules);

		/**
		 * @brief Set maximum number of LEDs per module
		 * @param max_leds Maximum LED count per module (1-16 for PCA9685)
		 * @return true if value is valid and set successfully
		 */
		bool setPca9685LedMax(uint8_t max_leds);

		/**
		 * @brief Set maximum LED name length
		 * @param max_length Maximum name length in characters
		 * @return true if value is valid and set successfully
		 */
		bool setLedNameMax(size_t max_length);

		// === Helper functions ===

		/**
		 * @brief Validate current configuration
		 * @return true if all configuration parameters are valid
		 */
		bool isValid() const;

		/**
		 * @brief Reset configuration to default values
		 */
		void reset();

		/**
		 * @brief Print current configuration to Serial
		 */
		void printConfiguration() const;
};

/**
 * @brief Global configuration instance
 * 
 * This global instance provides access to the system configuration
 * throughout the application. It should be initialized early in
 * the setup() function.
 */
extern Config config;