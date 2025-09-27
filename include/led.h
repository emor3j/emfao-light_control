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
 * @file    led.h
 * @brief   Declaration of the LED class.
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <Arduino.h>

#include "program.h"


/**
 * @brief LED controller class for managing individual LED properties and state
 * 
 * This class encapsulates all properties and behaviors of a single LED connected
 * to a PCA9685 PWM controller. It manages brightness, enable/disable state,
 * program execution, and provides a clean interface for LED manipulation.
 */
class LED {
	private:
		String name_;                     ///< User-friendly name for the LED
		uint16_t brightness_;             ///< Current brightness level (0-4095, 12-bit PWM)
		bool enabled_;                    ///< Enable/disable state of the LED
		ProgramType program_type_;        ///< Type of program currently running
		ProgramState* program_state_;     ///< Pointer to current program state

	public:
		// === Constructor and Destructor ===

		/**
		 * @brief Default constructor
		 * 
		 * Initializes LED with default values:
		 * - Name: empty string
		 * - Brightness: 0
		 * - Enabled: false
		 * - Program: PROGRAM_NONE
		 * - Program state: nullptr
		 */
		LED();

		/**
		 * @brief Parameterized constructor
		 * 
		 * @param name Initial name for the LED
		 * @param brightness Initial brightness value (0-4095)
		 * @param enabled Initial enable state
		 * @param program_type Initial assigned program
		 * @param program_state Initial program state
		 */
		LED(const String& name, uint16_t brightness, bool enabled,
			ProgramType program_type, ProgramState* program_state);

		/**
		 * @brief Copy constructor
		 * 
		 * @param other LED instance to copy from
		 */
		LED(const LED& other);

		/**
		 * @brief Assignment operator
		 * 
		 * @param other LED instance to assign from
		 * @return Reference to this instance
		 */
		LED& operator=(const LED& other);

		/**
		 * @brief Destructor
		 * 
		 * Cleans up program state if allocated
		 */
		~LED();


		// === Getters ===

		/**
		 * @brief Get the LED name
		 * 
		 * @return Current LED name as String
		 */
		const String& getName() const { return name_; }

		/**
		 * @brief Get current brightness level
		 * 
		 * @return Brightness value (0-4095)
		 */
		uint16_t getBrightness() const { return brightness_; }

		/**
		 * @brief Get enable state
		 * 
		 * @return true if LED is enabled, false otherwise
		 */
		bool isEnabled() const { return enabled_; }

		/**
		 * @brief Get current program type
		 * 
		 * @return Current ProgramType
		 */
		ProgramType getProgramType() const { return program_type_; }

		/**
		 * @brief Get program state pointer
		 * 
		 * @return Pointer to current ProgramState (can be nullptr)
		 */
		ProgramState* getProgramState() const { return program_state_; }


		// === Setters ===

		/**
		 * @brief Set LED name
		 * 
		 * @param name New name for the LED
		 */
		void setName(const String& name) { name_ = name; }

		/**
		 * @brief Set brightness level
		 * 
		 * Automatically clamps the value to valid range (0-4095)
		 * 
		 * @param brightness New brightness value
		 */
		void setBrightness(uint16_t brightness);

		/**
		 * @brief Set enable state
		 * 
		 * @param enabled New enable state
		 */
		void setEnabled(bool enabled) { enabled_ = enabled; }

		/**
		 * @brief Set program type and state
		 * 
		 * @param program_type New program type
		 * @param program_state Pointer to program state (can be nullptr)
		 */
		void setProgram(ProgramType program_type = PROGRAM_NONE,
			ProgramState* program_state = nullptr);

		
		// === Utility Methods ===

		/**
		 * @brief Get brightness as percentage
		 * 
		 * @return Brightness as percentage (0.0 - 100.0)
		 */
		float getBrightnessPercent() const;

		/**
		 * @brief Set brightness from percentage
		 * 
		 * @param percent Brightness percentage (0.0 - 100.0)
		 */
		void setBrightnessPercent(float percent);

		/**
		 * @brief Toggle enable state
		 * 
		 * @return New enable state after toggle
		 */
		bool toggle();

		/**
		 * @brief Reset LED to default state
		 * 
		 * Sets brightness to 0, disabled, and clears program
		 */
		void reset();

		/**
		 * @brief Check if LED has an active program
		 * 
		 * @return true if a program is running, false otherwise
		 */
		bool hasProgram() const;

		/**
		 * @brief Get effective brightness
		 * 
		 * Returns actual brightness taking into account enable state
		 * 
		 * @return Effective brightness (0 if disabled, actual brightness if enabled)
		 */
		uint16_t getEffectiveBrightness() const;


		// === Static Constants ===
		
		static constexpr uint16_t MIN_BRIGHTNESS = 0;      ///< Minimum brightness value
		static constexpr uint16_t MAX_BRIGHTNESS = 4095;   ///< Maximum brightness value (12-bit PWM)
};