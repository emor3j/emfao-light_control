/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file config.cpp
 * @brief Implementation of config class
 * 
 * This file implements the Config class for ESP32 LED controller system.
 *
 * See config.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include "config.h"
#include "pca9685.h"
#include "log.h"


/// Global configuration instance
Config config;


// === Constructor and Destructor ===

// Default constructor
Config::Config() : 
	i2c_pin_sda_(21),
	i2c_pin_scl_(22),
	pca9685_addr_min_(PCA9685Module::ADDR_MIN),
	pca9685_addr_max_(PCA9685Module::ADDR_MAX),
	pca9685_module_max_(PCA9685Module::MODULE_MAX),
	pca9685_led_max_(PCA9685Module::LED_MAX),
	led_name_max_(64) {}

// Parametric constructor
Config::Config(
	uint8_t sda_pin,
	uint8_t scl_pin,
	uint8_t addr_min,
	uint8_t addr_max,
	uint8_t module_max,
	uint8_t led_max,
	size_t name_max) :
	i2c_pin_sda_(sda_pin),
	i2c_pin_scl_(scl_pin),
	pca9685_addr_min_(addr_min),
	pca9685_addr_max_(addr_max),
	pca9685_module_max_(module_max),
	pca9685_led_max_(led_max),
	led_name_max_(name_max) {}

	
// === Setters with validation ===

bool Config::setI2cSdaPin(uint8_t pin) {
	if (isValidGpioPin(pin)) {
		i2c_pin_sda_ = pin;
		return true;
	}

	return false;
}

bool Config::setI2cSclPin(uint8_t pin) {
	if (isValidGpioPin(pin)) {
		i2c_pin_scl_ = pin;
		return true;
	}

	return false;
}

bool Config::setPca9685AddressRange(uint8_t min_addr, uint8_t max_addr) {
	if (min_addr <= max_addr && min_addr >= 0x08 && max_addr <= 0x77) {
		pca9685_addr_min_ = min_addr;
		pca9685_addr_max_ = max_addr;
		return true;
	}

	return false;
}

bool Config::setPca9685ModuleMax(uint8_t max_modules) {
	if (max_modules > 0 && max_modules <= PCA9685Module::MODULE_MAX) {
		pca9685_module_max_ = max_modules;
		return true;
	}

	return false;
}

bool Config::setPca9685LedMax(uint8_t max_leds) {
	if (max_leds > 0 && max_leds <= PCA9685Module::LED_MAX) {
		pca9685_led_max_ = max_leds;
		return true;
	}

	return false;
}

bool Config::setLedNameMax(size_t max_length) {
	if (max_length > 0 && max_length <= 256) { // Reasonable limit
		led_name_max_ = max_length;
		return true;
	}

	return false;
}


// === Helper functions ===

// Validation
bool Config::isValid() const {
	return isValidGpioPin(i2c_pin_sda_) &&
		isValidGpioPin(i2c_pin_scl_) &&
		i2c_pin_sda_ != i2c_pin_scl_ &&
		pca9685_addr_min_ <= pca9685_addr_max_ &&
		pca9685_addr_min_ >= PCA9685Module::ADDR_MIN &&
		pca9685_addr_max_ <= PCA9685Module::ADDR_MAX &&
		pca9685_module_max_ > 0 &&
		pca9685_module_max_ <= PCA9685Module::MODULE_MAX &&
		pca9685_led_max_ > 0 &&
		pca9685_led_max_ <= PCA9685Module::LED_MAX &&
		led_name_max_ > 0;
}

// Reset to defaults
void Config::reset() {
	*this = Config(); // Use default constructor
}

// Debug output
void Config::printConfiguration() const {
	LOG_INFO("[CONFIG] Current configuration:");
	LOG_INFO("[CONFIG] I2C - SDA: %d, SCL: %d\n", i2c_pin_sda_, i2c_pin_scl_);
	LOG_INFO("[CONFIG] PCA9685 - Addr range: 0x%02X-0x%02X\n", pca9685_addr_min_, pca9685_addr_max_);
	LOG_INFO("[CONFIG] Limits - Modules: %d, LEDs/module: %d\n", pca9685_module_max_, pca9685_led_max_);
	LOG_INFO("[CONFIG] LED name max length: %zu\n", led_name_max_);
	LOG_INFO("[CONFIG] Configuration is %s\n", isValid() ? "VALID" : "INVALID");
}


// === Private static method ===

bool Config::isValidGpioPin(uint8_t pin) {
	// ESP32 valid GPIO pins (excluding input-only and special pins)
	return (
		pin <= 33 &&
		pin != 6 &&
		pin != 7 &&
		pin != 8 && 
		pin != 9 &&
		pin != 10 &&
		pin != 11
	);
}