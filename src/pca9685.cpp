/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 * 
 * @file pca9685.cpp
 * @brief Implementation of PCA9685Module and ModuleManager class
 * 
 * This file implements the PCA9685Module and ModuleManager class
 * for ESP32 LED controller system.
 *
 * See pca9685.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include "pca9685.h"
#include "config.h"
#include "storage.h"
#include "driver/i2c.h"
#include "log.h"


// Global instance
std::unique_ptr<ModuleManager> module_manager;

// =============================================================================
// PCA9685Module Implementation
// =============================================================================

// === Constructor and Destructor ===

// Default constructor
PCA9685Module::PCA9685Module(uint8_t address, uint8_t led_count) :
	address_(address),
	detected_(false),
	initialized_(false),
	name_(generateDefaultName()),
	led_count_(led_count),
	leds_(nullptr),
	driver_(nullptr) {
	// Allocate LED array
	leds_.reset(new LED[led_count]);
}

// Destructor
PCA9685Module::~PCA9685Module() {
	// Smart pointers will handle cleanup automatically
}

// Move constructor
PCA9685Module::PCA9685Module(PCA9685Module&& other) noexcept :
	address_(other.address_),
	detected_(other.detected_),
	initialized_(other.initialized_),
	name_(std::move(other.name_)),
	led_count_(other.led_count_),
	leds_(std::move(other.leds_)),
	driver_(std::move(other.driver_)) {
	// Reset other object
	other.address_ = 0;
	other.detected_ = false;
	other.initialized_ = false;
	other.led_count_ = 0;
}

// Assign constructor
PCA9685Module& PCA9685Module::operator=(PCA9685Module&& other) noexcept {
	if (this != &other) {
		address_ = other.address_;
		detected_ = other.detected_;
		initialized_ = other.initialized_;
		name_ = std::move(other.name_);
		led_count_ = other.led_count_;
		leds_ = std::move(other.leds_);
		driver_ = std::move(other.driver_);
		
		// Reset other object
		other.address_ = 0;
		other.detected_ = false;
		other.initialized_ = false;
		other.led_count_ = 0;
	}
	return *this;
}

// === Getters ==

LED* PCA9685Module::getLED(uint8_t led_index) {
	if (led_index >= led_count_ || !leds_) {
		return nullptr;
	}
	return &leds_[led_index];
}

const LED* PCA9685Module::getLED(uint8_t led_index) const {
	if (led_index >= led_count_ || !leds_) {
		return nullptr;
	}
	return &leds_[led_index];
}

// === Other functions ===

bool PCA9685Module::initialize() {
	if (initialized_) {
		return true; // Already initialized
	}
	
	LOG_INFO("[PCA9685] Initializing module %s...\n", name_.c_str());

	// Create driver instance
	driver_.reset(new Adafruit_PWMServoDriver(address_));
	
	if (!driver_) {
		LOG_ERROR("[PCA9685] Failed to create driver instance\n");
		return false;
	}
	
	// Initialize driver
	driver_->begin();
	
	// Test initialization by setting oscillator frequency and PWM frequency
	bool init_success = true;
	try {
		driver_->setOscillatorFrequency(27000000);
		driver_->setPWMFreq(1600); // Good frequency for LEDs
	} catch (...) {
		init_success = false;
	}
	
	if (!init_success) {
		LOG_ERROR("[PCA9685] Failed to initialize PCA9685 at 0x%02X\n", address_);
		driver_.reset(); // Clean up
		return false;
	}
	
	initialized_ = true;
	
	LOG_INFO("[PCA9685] PCA9685 at 0x%02X initialized successfully\n", address_);
	
	return true;
}

bool PCA9685Module::applyLedBrightness(uint8_t led_index) {
	if (!initialized_ || !driver_ || led_index >= led_count_ || !leds_) {
		return false;
	}
	
	const LED* led = getLED(led_index);
	if (!led) {
		return false;
	}
	
	// FULL OFF case
	if (!led->isEnabled() || led->getBrightness() == 0) {
		driver_->setPWM(led_index, 0, 4096);
		return true;
	}
	
	// FULL ON case
	if (led->getBrightness() == LED::MAX_BRIGHTNESS) {
		driver_->setPWM(led_index, 4096, 0);
		return true;
	}
	
	// Normal PWM case
	driver_->setPWM(led_index, 0, led->getBrightness());
	return true;
}

void PCA9685Module::setupDefaultLeds(uint8_t module_index) {
	if (!leds_) {
		return;
	}
	
	for (uint8_t led_index = 0; led_index < led_count_; led_index++) {
		// Create LED name following convention
		String led_name = "LED_" + String(module_index) + "_" + String(led_index);
		
		// Initialize LED with default values
		leds_[led_index].setName(led_name);
		leds_[led_index].setBrightness(0);
		leds_[led_index].setEnabled(false);
		leds_[led_index].setProgram(PROGRAM_NONE, nullptr);
		
		// Apply brightness to hardware
		applyLedBrightness(led_index);
	}
}

bool PCA9685Module::isPCA9685Device(uint8_t address) {
	// Simple check: try to read the MODE1 register
	Wire.beginTransmission(address);
	Wire.write(0x00); // MODE1 register
	uint8_t error = Wire.endTransmission();
	
	if (error != 0) {
		return false;
	}
	
	Wire.requestFrom(address, static_cast<uint8_t>(1));
	if (Wire.available()) {
		uint8_t mode1 = Wire.read();
		// MODE1 register should have reasonable values
		return (mode1 & 0x80) == 0; // RESTART bit should be 0 normally
	}
	
	return false;
}

// === Private functions ===

String PCA9685Module::generateDefaultName() const {
	return "PCA9685_" + String(address_, HEX);
}


// =============================================================================
// ModuleManager Implementation
// =============================================================================


// === Constructor and Destructor ===

// Default constructor
ModuleManager::ModuleManager() {
	modules_.reserve(16); // Reserve space for up to 16 modules
}

// === Getters ===

PCA9685Module* ModuleManager::getModule(uint8_t index) {
	if (index >= modules_.size()) {
		return nullptr;
	}
	return modules_[index].get();
}

const PCA9685Module* ModuleManager::getModule(uint8_t index) const {
	if (index >= modules_.size()) {
		return nullptr;
	}
	return modules_[index].get();
}

LED* ModuleManager::getLED(uint8_t module_index, uint8_t led_index) {
	PCA9685Module* module = getModule(module_index);
	if (!module) {
		return nullptr;
	}
	return module->getLED(led_index);
}

const LED* ModuleManager::getLED(uint8_t module_index, uint8_t led_index) const {
	const PCA9685Module* module = getModule(module_index);
	if (!module) {
		return nullptr;
	}
	return module->getLED(led_index);
}

uint16_t ModuleManager::getTotalLedCount() const {
	uint16_t total = 0;
	for (const auto& module : modules_) {
		if (module) {
			total += module->getLedCount();
		}
	}
	return total;
}

uint8_t ModuleManager::getInitializedModuleCount() const {
	uint8_t count = 0;
	for (const auto& module : modules_) {
		if (module && module->isInitialized()) {
			count++;
		}
	}
	return count;
}

uint16_t ModuleManager::getEnabledLedCount() const {
	uint16_t count = 0;
	for (const auto& module : modules_) {
		if (module) {
			for (uint8_t j = 0; j < module->getLedCount(); j++) {
				const LED* led = module->getLED(j);
				if (led && led->isEnabled()) {
					count++;
				}
			}
		}
	}
	return count;
}

// === Other functions ===

bool ModuleManager::initialize() {
	LOG_INFO("[MODULEMGR] Setting up PCA9685 modules...\n");
	
	// Clear existing modules
	modules_.clear();
	
	// Scan for modules
	uint8_t found_count = scanModules();
	if (found_count == 0) {
		LOG_ERROR("[MODULEMGR] No PCA9685 modules found\n");
		return false;
	}
	
	// Initialize modules
	uint8_t initialized_count = initializeModules();
	
	LOG_INFO("[MODULEMGR] PCA9685 modules initialized: %d/%d\n", initialized_count, found_count);
	
	return initialized_count > 0;
}

bool ModuleManager::applyLedBrightness(uint8_t module_index, uint8_t led_index) {
	PCA9685Module* module = getModule(module_index);
	if (!module) {
		return false;
	}
	return module->applyLedBrightness(led_index);
}

void ModuleManager::printModuleInfo() const {
	LOG_INFO("[MODULEMGR] === PCA9685 Module Information ===\n");
	LOG_INFO("[MODULEMGR] Total modules: %d\n", modules_.size());
	LOG_INFO("[MODULEMGR] Initialized modules: %d\n", getInitializedModuleCount());
	LOG_INFO("[MODULEMGR] Total LEDs: %d\n", getTotalLedCount());

	for (size_t i = 0; i < modules_.size(); i++) {
		const auto& module = modules_[i];
		if (module) {
			LOG_INFO("[MODULEMGR] Module %d: %s (0x%02X) - %s - %d LEDs\n",
				i,
				module->getName(),
				module->getAddress(),
				module->isInitialized() ? "INITIALIZED" : "FAILED",
				module->getLedCount());
		}
	}
}

// === Private functions ===

uint8_t ModuleManager::scanModules() {
	LOG_INFO("[MODULEMGR] Scanning for PCA9685 modules...\n");
	
	uint8_t found_count = 0;
	
	for (uint8_t addr = config.getPca9685AddrMin(); 
		addr <= config.getPca9685AddrMax() && found_count < config.getPca9685ModuleMax(); 
		addr++) {
		
		Wire.beginTransmission(addr);
		uint8_t error = Wire.endTransmission();
		
		if (error == 0) {
			// Device found, check if it's a PCA9685
			if (PCA9685Module::isPCA9685Device(addr)) {
				// Create module instance
				std::unique_ptr<PCA9685Module> module(new PCA9685Module(addr, config.getPca9685LedMax()));
				module->setDetected(true); // Mark as detected
				
				LOG_INFO("[MODULEMGR] PCA9685 found at address 0x%02X\n", addr);
				
				modules_.push_back(std::move(module));
				found_count++;
			}
		}
	}
	
	LOG_INFO("[MODULEMGR] Total PCA9685 modules detected: %d\n", found_count);
	
	return found_count;
}

uint8_t ModuleManager::initializeModules() {
	uint8_t initialized_count = 0;
	
	for (size_t i = 0; i < modules_.size(); i++) {
		auto& module = modules_[i];
		if (module && module->initialize()) {
			// Setup default LEDs with proper module index
			module->setupDefaultLeds(i);
			initialized_count++;
		}
	}
	
	return initialized_count;
}