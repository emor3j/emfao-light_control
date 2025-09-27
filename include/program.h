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
 * @file    program.h
 * @brief   Declaration of the ProgramManager class.
 * 
 * This header defines the program management system that allows assigning
 * and controlling various animated effects to individual LEDs connected
 * to PCA9685 PWM controllers.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <memory>


/**
 * @enum ProgramType
 * @brief Enumeration of available LED program types
 * 
 * Defines the different types of animated programs that can be assigned
 * to LEDs for creating various visual effects.
 */
enum ProgramType {
	PROGRAM_NONE = 0,           ///< No program assigned
	PROGRAM_WELDING = 1,        ///< Welding arc simulation with random flashes
	PROGRAM_HEARTBEAT = 2,      ///< Heartbeat rhythm with double pulse pattern
	PROGRAM_BREATHING = 3,      ///< Breathing effect with smooth fade in/out
	PROGRAM_SIMPLE_BLINK = 4,   ///< Simple 1 second on/off blinking
	PROGRAM_TV_FLICKER = 5,     ///< TV screen flicker simulation
	PROGRAM_FIREBOX_GLOW = 6,   ///< Firebox glow simulation
	PROGRAM_CANDLE_FLICKER = 7, ///< Candle flame flickering simulation
	PROGRAM_FRENCH_CROSSING = 8 ///< French level crossing light with filament bulb effect
};

/**
 * @struct ProgramState
 * @brief State information for a running LED program
 * 
 * Contains all the runtime state information needed to manage
 * the execution of an LED program, including timing, intensity,
 * and custom parameters.
 */
struct ProgramState {
	unsigned long last_update;    ///< Timestamp of last program update
	unsigned long next_event;     ///< Timestamp for next scheduled event
	bool active;                  ///< Whether the program is currently active
	unsigned long start_time;     ///< Program start timestamp for cycle calculations
	uint16_t current_intensity;   ///< Current LED intensity (0-4095)
	JsonDocument parameters;      ///< Custom program parameters (extensible)
};

/**
 * @class ProgramManager
 * @brief Static class for managing LED programs across all modules
 * 
 * The ProgramManager provides a centralized system for assigning, updating,
 * and managing animated LED programs. It works directly with the LED objects
 * in the PCA9685 modules and handles all timing and state management.
 * 
 * @note All methods are static as there should be only one program manager
 *       instance in the system.
 */
class ProgramManager {
	public:
		/**
		 * @brief Initialize the program manager
		 * 
		 * Sets up the program manager and prepares it for operation.
		 * Must be called once during system initialization.
		 * 
		 * @return true if initialization successful, false otherwise
		 */
		static bool initialize();
		
		/**
		 * @brief Update all active LED programs
		 * 
		 * This method should be called regularly (typically in the main loop)
		 * to update all active LED programs. It handles timing, state transitions,
		 * and applies brightness changes to the hardware.
		 * 
		 * @param current_millis Current system time in milliseconds
		 * 
		 * @note Recommended call frequency: 100Hz (every 10ms) for smooth effects
		 */
		static void update(unsigned long current_millis);
		
		// === Program Assignment Management ===
		
		/**
		 * @brief Assign a program to a specific LED
		 * 
		 * Assigns the specified program type to the LED at the given module
		 * and LED coordinates. Automatically initializes the program state
		 * and starts execution if the LED is enabled.
		 * 
		 * @param module_id PCA9685 module index (0-based)
		 * @param led_id LED index within the module (0-based)
		 * @param program_type Type of program to assign
		 * 
		 * @return true if assignment successful, false if invalid coordinates
		 *         or program type
		 * 
		 * @note If a program is already assigned to the LED, it will be
		 *       replaced and its state cleaned up
		 */
		static bool assign_program(uint8_t module_id, uint8_t led_id, ProgramType program_type);
		
		/**
		 * @brief Remove program assignment from a specific LED
		 * 
		 * Unassigns any program from the specified LED, cleans up its state,
		 * and sets the LED to no program (PROGRAM_NONE).
		 * 
		 * @param module_id PCA9685 module index (0-based)
		 * @param led_id LED index within the module (0-based)
		 * 
		 * @return true if unassignment successful, false if invalid coordinates
		 */
		static bool unassign_program(uint8_t module_id, uint8_t led_id);
		
		/**
		 * @brief Check if a program is assigned to a specific LED
		 * 
		 * @param module_id PCA9685 module index (0-based)
		 * @param led_id LED index within the module (0-based)
		 * 
		 * @return true if a program (other than PROGRAM_NONE) is assigned,
		 *         false otherwise or if invalid coordinates
		 */
		static bool is_program_assigned(uint8_t module_id, uint8_t led_id);
		
		/**
		 * @brief Get the program type assigned to a specific LED
		 * 
		 * @param module_id PCA9685 module index (0-based)
		 * @param led_id LED index within the module (0-based)
		 * 
		 * @return ProgramType assigned to the LED, or PROGRAM_NONE if no program
		 *         is assigned or coordinates are invalid
		 */
		static ProgramType get_program_type(uint8_t module_id, uint8_t led_id);
		
		// === Program Information ===
		
		/**
		 * @brief Get information about all available program types
		 * 
		 * Returns a JSON document containing information about all supported
		 * program types, including their IDs, names, and descriptions.
		 * 
		 * @return JsonDocument with structure:
		 *         {
		 *           "programs": [
		 *             {
		 *               "id": <program_id>,
		 *               "name": "<program_name>",
		 *               "description": "<program_description>"
		 *             },
		 *             ...
		 *           ],
		 *           "total": <count>
		 *         }
		 */
		static JsonDocument get_available_programs();
		
		/**
		 * @brief Get information about all currently assigned programs
		 * 
		 * Returns a JSON document containing information about all LEDs
		 * that currently have programs assigned to them.
		 * 
		 * @return JsonDocument with structure:
		 *         {
		 *           "assigned_programs": [
		 *             {
		 *               "module_id": <module_id>,
		 *               "led_id": <led_id>,
		 *               "program_type": <program_type_id>,
		 *               "program_name": "<program_name>",
		 *               "enabled": <boolean>
		 *             },
		 *             ...
		 *           ],
		 *           "total": <count>
		 *         }
		 */
		static JsonDocument get_assigned_programs();
		
		/**
		 * @brief Get human-readable name for a program type
		 * 
		 * @param type Program type to get name for
		 * @return String containing the program name, or "None" for PROGRAM_NONE
		 */
		static String get_program_name(ProgramType type);
		
		/**
		 * @brief Get description for a program type
		 * 
		 * @param type Program type to get description for
		 * @return String containing the program description
		 */
		static String get_program_description(ProgramType type);
		
		// === Persistence ===
		
		/**
		 * @brief Clear all program assignments
		 * 
		 * Removes all program assignments from all LEDs and cleans up
		 * their associated state.
		 */
		static void clear_programs();

		/**
		 * @brief Initialize program state of a LED
		 * 
		 * Sets up program state for a LED.
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 */
		static bool initialize_led_state(uint8_t module_id, uint8_t led_id);
	
	private:
		// === Program Update Methods ===
		
		/**
		 * @brief Update welding program for a specific LED
		 * 
		 * Handles the welding arc simulation with random intensity flashes
		 * and realistic timing patterns.
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 * @param current_millis Current system time in milliseconds
		 */
		static void update_welding_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis);
		
		/**
		 * @brief Update heartbeat program for a specific LED
		 * 
		 * Handles the double-pulse heartbeat rhythm with systole/diastole pattern.
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 * @param current_millis Current system time in milliseconds
		 */
		static void update_heartbeat_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis);
		
		/**
		 * @brief Update breathing program for a specific LED
		 * 
		 * Handles the smooth breathing effect with sinusoidal fade in/out
		 * pattern including inhale, hold, exhale, and pause phases.
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 * @param current_millis Current system time in milliseconds
		 */
		static void update_breathing_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis);

		/**
		 * @brief Update simple blink program for a specific LED
		 * 
		 * Handles simple on/off blinking with 1 second intervals.
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 * @param current_millis Current system time in milliseconds
		 */
		static void update_simple_blink_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis);

		/**
		 * @brief Update TV flicker program for a specific LED
		 * 
		 * Simulates the random flickering of a television screen with
		 * blue-tinted light and irregular intensity changes.
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 * @param current_millis Current system time in milliseconds
		 */
		static void update_tv_flicker_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis);

		/**
		 * @brief Firebox glow program for a specific LED
		 * 
		 * Simulates a wood fire with crackling flames, ember pops,
		 * and natural burning variations with wind effects.
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 * @param current_millis Current system time in milliseconds
		 */
		static void update_firebox_glow_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis);

		/**
		 * @brief Update candle flicker program for a specific LED
		 * 
		 * Simulates the gentle flickering of a candle or gas lamp flame
		 * with organic variations and occasional stronger flickers.
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 * @param current_millis Current system time in milliseconds
		 */
		static void update_candle_flicker_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis);

		/**
		 * @brief Update French level crossing program for a specific LED
		 * 
		 * Simulates a French railway level crossing warning light with
		 * realistic filament bulb behavior: gradual warm-up and instant extinction.
		 * Follows the French standard of 1Hz blinking (0.5s ON, 0.5s OFF).
		 * 
		 * @param module_id PCA9685 module index
		 * @param led_id LED index within module
		 * @param current_millis Current system time in milliseconds
		 */
		static void update_french_crossing_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis);
		
		// === State Initialization Methods ===
		
		/**
		 * @brief Initialize default program state
		 * 
		 * Sets up initial state for default program.
		 * 
		 * @param state Pointer to ProgramState to initialize
		 */
		static void initialize_default_state(ProgramState* state);

		/**
		 * @brief Initialize welding program state
		 * 
		 * Sets up initial state for welding program including random
		 * timing for the first flash.
		 * 
		 * @param state Pointer to ProgramState to initialize
		 */
		static void initialize_welding_state(ProgramState* state);		
};

/// Global instance of the program manager
/**
 * @brief Global ProgramManager instance
 * 
 * This global instance provides access to the program system
 * throughout the application. It should be initialized early in
 * the setup() function.
 */
extern std::unique_ptr<ProgramManager> program_manager;