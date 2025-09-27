/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file led.cpp
 * @brief Implementation of LED class
 * 
 * This file implements the LED class.
 *
 * See led.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include "led.h"
#include "config.h"
#include "program.h"
#include "pca9685.h"


// === Constructor and Destructor ===

// Default constructor
LED::LED() :
	name_(""), 
	brightness_(0),
	enabled_(false),
	program_type_(PROGRAM_NONE),
	program_state_(nullptr) {}

// Parametric constructor
LED::LED(
	const String& name,
	uint16_t brightness,
	bool enabled,
	ProgramType program_type,
	ProgramState* program_state) :
	name_(name), 
	brightness_(brightness),
	enabled_(enabled),
	program_type_(program_type),
	program_state_(program_state) {}

// Copy constructor
LED::LED(const LED& other) :
	name_(other.name_),
	brightness_(other.brightness_),
	enabled_(other.enabled_),
	program_type_(other.program_type_),
	program_state_(nullptr) {}

// Assignment operator
LED& LED::operator=(const LED& other) {
	if (this != &other) {
		name_ = other.name_;
		brightness_ = other.brightness_;
		enabled_ = other.enabled_;
		program_type_ = other.program_type_;
		program_state_ = nullptr;
	}

	return *this;
}

// Destructor
LED::~LED() {
	delete program_state_;
}

// === Setters ===

void LED::setBrightness(uint16_t brightness) {
	// Clamp brightness to valid range
	if (brightness > MAX_BRIGHTNESS) {
		brightness_ = MAX_BRIGHTNESS;
	} else {
		brightness_ = brightness;
	}
}

void LED::setProgram(ProgramType program_type, ProgramState* program_state) {
	program_type_ = program_type;
	program_state_ = program_state;
}

// === Utility Methods ===

float LED::getBrightnessPercent() const {
	return (static_cast<float>(brightness_) / MAX_BRIGHTNESS) * 100.0f;
}

void LED::setBrightnessPercent(float percent) {
	// Clamp percentage to valid range
	if (percent < 0.0f) percent = 0.0f;
	if (percent > 100.0f) percent = 100.0f;
	
	// Convert percentage to brightness value
	uint16_t brightness = static_cast<uint16_t>((percent / 100.0f) * MAX_BRIGHTNESS);
	setBrightness(brightness);
}

bool LED::toggle() {
	enabled_ = !enabled_;
	return enabled_;
}

void LED::reset() {
	brightness_ = 0;
	enabled_ = false;
	program_type_ = PROGRAM_NONE;
	program_state_ = nullptr;
}

bool LED::hasProgram() const {
	return (
		program_type_ != PROGRAM_NONE && 
		program_state_ != nullptr
	);
}

uint16_t LED::getEffectiveBrightness() const {
	return enabled_ ? brightness_ : 0; 
}