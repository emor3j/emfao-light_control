/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file program.cpp
 * @brief Implementation of LED program management system
 * 
 * This file implements the ProgramManager class and all LED animation programs.
 * It provides various visual effects including welding simulation, heartbeat,
 * railway signals, level crossing warnings, and breathing effects.
 * 
 * The program manager works directly with LED objects in PCA9685 modules to
 * control brightness and create smooth animations with realistic timing.
 * 
 * See program.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include "program.h"
#include "config.h"
#include "pca9685.h"
#include "storage.h"
#include "log.h"


/// Global instance
std::unique_ptr<ProgramManager> program_manager;

// === Welding Program Parameters ===
/// @defgroup welding_params Welding Program Parameters
/// @brief Configuration constants for the welding arc simulation effect
/// @{

/// Minimum interval between welding flashes (milliseconds)
const unsigned long WELDING_MIN_INTERVAL = 10;
/// Maximum interval between welding flashes (milliseconds)
const unsigned long WELDING_MAX_INTERVAL = 300;
/// Minimum duration of a welding flash (milliseconds)
const unsigned long WELDING_MIN_DURATION = 10;
/// Maximum duration of a welding flash (milliseconds)
const unsigned long WELDING_MAX_DURATION = 100;
/// Minimum intensity for welding flashes (0-4095)
const uint16_t WELDING_MIN_INTENSITY = 10;
/// Maximum intensity for welding flashes (0-4095)
const uint16_t WELDING_MAX_INTENSITY = 3000;

/// @}

// === Heartbeat Program Parameters ===
/// @defgroup heartbeat_params Heartbeat Program Parameters
/// @brief Configuration constants for the heartbeat rhythm effect
/// @{

/// Total duration of one complete heartbeat cycle (milliseconds)
const unsigned long HEARTBEAT_CYCLE_DURATION = 1000;
/// Duration of the first heartbeat pulse (systole) (milliseconds)
const unsigned long HEARTBEAT_BEAT1_DURATION = 100;
/// Duration of pause after first beat (milliseconds)
const unsigned long HEARTBEAT_PAUSE1_DURATION = 80;
/// Duration of the second heartbeat pulse (diastole) (milliseconds)
const unsigned long HEARTBEAT_BEAT2_DURATION = 60;
/// Duration of pause after second beat (milliseconds)
const unsigned long HEARTBEAT_PAUSE2_DURATION = 760;
/// Maximum intensity for heartbeat pulses (0-4095)
const uint16_t HEARTBEAT_INTENSITY = 3500;

/// @}

// === Breathing Program Parameters ===
/// @defgroup breathing_params Breathing Program Parameters
/// @brief Configuration constants for the breathing effect
/// @{

/// Total duration of one complete breathing cycle (milliseconds)
const unsigned long BREATHING_CYCLE_DURATION = 4000;
/// Duration of the inhale phase (fade in) (milliseconds)
const unsigned long BREATHING_INHALE_DURATION = 1500;
/// Duration of holding breath at maximum intensity (milliseconds)
const unsigned long BREATHING_HOLD_DURATION = 500;
/// Duration of the exhale phase (fade out) (milliseconds)
const unsigned long BREATHING_EXHALE_DURATION = 1500;
/// Duration of pause at minimum intensity (milliseconds)
const unsigned long BREATHING_PAUSE_DURATION = 500;
/// Maximum intensity during breathing cycle (0-4095)
const uint16_t BREATHING_MAX_INTENSITY = 4095;
/// Minimum intensity during breathing cycle (0-4095)
const uint16_t BREATHING_MIN_INTENSITY = 0;

/// @}

// === Simple Blink Program Parameters ===
/// @defgroup blink_params Simple Blink Program Parameters
/// @brief Configuration constants for the simple blinking effect
/// @{

/// Duration of ON phase (milliseconds)
const unsigned long SIMPLE_BLINK_ON_DURATION = 1000;
/// Duration of OFF phase (milliseconds)
const unsigned long SIMPLE_BLINK_OFF_DURATION = 1000;
/// Intensity for blink ON state (0-4095)
const uint16_t SIMPLE_BLINK_INTENSITY = 4095;

/// @}

// === TV Flicker Program Parameters ===
/// @defgroup tv_flicker_params TV Flicker Program Parameters
/// @brief Configuration constants for TV screen flickering effect
/// @{

/// Base intensity for TV flicker (0-4095)
const uint16_t TV_FLICKER_BASE_INTENSITY = 800;
/// Maximum intensity for TV flicker (0-4095)
const uint16_t TV_FLICKER_MAX_INTENSITY = 2500;
/// Minimum intensity for TV flicker (0-4095)
const uint16_t TV_FLICKER_MIN_INTENSITY = 200;
/// Minimum interval between flicker changes (milliseconds)
const unsigned long TV_FLICKER_MIN_INTERVAL = 40;
/// Maximum interval between flicker changes (milliseconds)
const unsigned long TV_FLICKER_MAX_INTERVAL = 200;
/// Probability of a bright flash (0-100)
const uint8_t TV_FLICKER_FLASH_PROBABILITY = 15;
/// Probability of a dim period (0-100)
const uint8_t TV_FLICKER_DIM_PROBABILITY = 10;

/// @}

// === Firebox Glow Program Parameters ===
/// @defgroup firebox_params Firebox Glow Program Parameters
/// @brief Configuration constants for wood fire simulation effect
/// @{

/// Base intensity for wood fire (0-4095)
const uint16_t FIREBOX_BASE_INTENSITY = 2200;
/// Maximum intensity for wood fire (0-4095)
const uint16_t FIREBOX_MAX_INTENSITY = 4095;
/// Minimum intensity for wood fire (0-4095)
const uint16_t FIREBOX_MIN_INTENSITY = 1200;
/// Minimum interval between flame changes (milliseconds)
const unsigned long FIREBOX_MIN_INTERVAL = 60;
/// Maximum interval between flame changes (milliseconds)
const unsigned long FIREBOX_MAX_INTERVAL = 400;
/// Probability of ember pop/crack (0-100)
const uint8_t FIREBOX_EMBER_POP_PROBABILITY = 15;
/// Probability of strong flame surge (0-100)
const uint8_t FIREBOX_FLAME_SURGE_PROBABILITY = 8;
/// Probability of wind gust effect (0-100)
const uint8_t FIREBOX_WIND_GUST_PROBABILITY = 5;
/// Duration of ember pop effect (milliseconds)
const unsigned long FIREBOX_EMBER_DURATION = 150;
/// Duration of flame surge effect (milliseconds)
const unsigned long FIREBOX_SURGE_DURATION = 800;
/// Duration of wind gust effect (milliseconds)
const unsigned long FIREBOX_WIND_DURATION = 1200;

/// @}

// === Candle Flicker Program Parameters ===
/// @defgroup candle_params Candle Flicker Program Parameters
/// @brief Configuration constants for candle flame flickering effect
/// @{

/// Base intensity for candle flame (0-4095)
const uint16_t CANDLE_BASE_INTENSITY = 2800;
/// Maximum intensity for candle flame (0-4095)
const uint16_t CANDLE_MAX_INTENSITY = 3800;
/// Minimum intensity for candle flame (0-4095)
const uint16_t CANDLE_MIN_INTENSITY = 1800;
/// Minimum interval between flicker changes (milliseconds)
const unsigned long CANDLE_MIN_INTERVAL = 50;
/// Maximum interval between flicker changes (milliseconds)
const unsigned long CANDLE_MAX_INTERVAL = 300;
/// Probability of a strong flicker (0-100)
const uint8_t CANDLE_STRONG_FLICKER_PROBABILITY = 12;
/// Probability of a gentle dip (0-100)
const uint8_t CANDLE_DIP_PROBABILITY = 8;

/// @}

// === French Level Crossing Program Parameters ===
/// @defgroup french_crossing_params French Level Crossing Program Parameters
/// @brief Configuration constants for French railway level crossing light simulation
/// @{

/// Duration of ON phase (milliseconds)
const unsigned long FRENCH_CROSSING_ON_DURATION = 500;
/// Duration of OFF phase (milliseconds)
const unsigned long FRENCH_CROSSING_OFF_DURATION = 500;
/// Maximum intensity for French crossing light (0-4095)
const uint16_t FRENCH_CROSSING_MAX_INTENSITY = 4095;
/// Duration of filament warm-up (milliseconds)
const unsigned long FRENCH_CROSSING_WARMUP_DURATION = 100;
/// Duration of filament cool-down (milliseconds)
const unsigned long FRENCH_CROSSING_COOLDOWN_DURATION = 150;
/// Minimum intensity during warm-up (0-4095)
const uint16_t FRENCH_CROSSING_WARMUP_MIN = 0;

/// @}

bool ProgramManager::initialize() {
	// Get assigned program
	JsonDocument assigned = get_assigned_programs();
	JsonArray programs = assigned["assigned_programs"];
	
	// Initialise all assigned program
	for (JsonObject program : programs) {
		uint8_t module_id = program["module_id"];
		uint8_t led_id = program["led_id"];
		ProgramType program_type = program["program_type"];

		LOG_DEBUG("[PROGRAMMGR] Initializing LED %d:%d with program %d (%s)\n",
			module_id,
			led_id,
			(uint8_t)program_type,
			get_program_name(program_type).c_str()
		);
		// Get LED
		const PCA9685Module* module = module_manager->getModule(module_id);
		if (!module || led_id >= module->getLedCount()) {
			return false;
		}
		
		LED* led_info = module_manager->getLED(module_id, led_id);
		if (!led_info) {
			return false;
		}

		// Create new program state
		if (led_info->getProgramState() != nullptr) {
			delete led_info->getProgramState();
		}
		led_info->setProgram(program_type, new ProgramState());
		initialize_led_state(module_id, led_id);
	}

	return true;
}

void ProgramManager::update(unsigned long current_millis) {
	if (!module_manager) return;
	
	for (uint8_t i = 0; i < module_manager->getModuleCount(); i++) {
		const PCA9685Module* module = module_manager->getModule(i);
		if (!module) continue;
		
		for (uint8_t j = 0; j < module->getLedCount(); j++) {
			LED* led_info = module_manager->getLED(i, j);
			if (!led_info) continue;
			
			// If LED is active and has assigned program
			if (led_info->isEnabled() && led_info->getProgramType() != PROGRAM_NONE && led_info->getProgramState() != nullptr) {
				switch (led_info->getProgramType()) {
					case PROGRAM_WELDING:
						update_welding_program(i, j, current_millis);
						break;
					case PROGRAM_HEARTBEAT:
						update_heartbeat_program(i, j, current_millis);
						break;
					case PROGRAM_BREATHING:
						update_breathing_program(i, j, current_millis);
						break;
					case PROGRAM_SIMPLE_BLINK:
						update_simple_blink_program(i, j, current_millis);
						break;
					case PROGRAM_TV_FLICKER:
						update_tv_flicker_program(i, j, current_millis);
						break;
					case PROGRAM_FIREBOX_GLOW:
						update_firebox_glow_program(i, j, current_millis);
						break;
					case PROGRAM_CANDLE_FLICKER:
						update_candle_flicker_program(i, j, current_millis);
						break;
					case PROGRAM_FRENCH_CROSSING:
						update_french_crossing_program(i, j, current_millis);
						break;
					default:
						break;
				}
			}
		}
	}
}

bool ProgramManager::assign_program(uint8_t module_id, uint8_t led_id, ProgramType program_type) {
	if (!module_manager || module_id >= module_manager->getModuleCount()) {
		return false;
	}
	
	const PCA9685Module* module = module_manager->getModule(module_id);
	if (!module || led_id >= module->getLedCount()) {
		return false;
	}
	
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) {
		return false;
	}
	
	if (program_type == PROGRAM_NONE) {
		return unassign_program(module_id, led_id);
	}
	
	// Create new program state
	if (led_info->getProgramState() != nullptr) {
		delete led_info->getProgramState();
	}
	led_info->setProgram(program_type, new ProgramState());
	initialize_led_state(module_id, led_id);
	
	LOG_INFO("[PROGRAMMGR] Program %d(%s) assigned to LED %d:%d\n", 
		program_type,
		get_program_name(program_type).c_str(), 
		module_id, 
		led_id);
	
	return true;
}

bool ProgramManager::unassign_program(uint8_t module_id, uint8_t led_id) {
	if (!module_manager || module_id >= module_manager->getModuleCount()) {
		return false;
	}
	
	const PCA9685Module* module = module_manager->getModule(module_id);
	if (!module || led_id >= module->getLedCount()) {
		return false;
	}
	
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) {
		return false;
	}
	
	// Delete program state memory
	if (led_info->getProgramState() != nullptr) {
		delete led_info->getProgramState();
		led_info->setProgram(led_info->getProgramType(), nullptr);
	}
	
	led_info->setProgram(PROGRAM_NONE, led_info->getProgramState());
	
	LOG_INFO("[PROGRAMMGR] Program unassigned from LED %d:%d\n", module_id, led_id);
	
	return true;
}

bool ProgramManager::is_program_assigned(uint8_t module_id, uint8_t led_id) {
	if (!module_manager || module_id >= module_manager->getModuleCount()) {
		return false;
	}
	
	const LED* led = module_manager->getLED(module_id, led_id);
	return led ? (led->getProgramType() != PROGRAM_NONE) : false;
}

ProgramType ProgramManager::get_program_type(uint8_t module_id, uint8_t led_id) {
	if (!module_manager || module_id >= module_manager->getModuleCount()) {
		return PROGRAM_NONE;
	}
	
	const LED* led = module_manager->getLED(module_id, led_id);
	return led ? led->getProgramType() : PROGRAM_NONE;
}

JsonDocument ProgramManager::get_available_programs() {
	JsonDocument doc;
	JsonArray programs = doc["programs"].to<JsonArray>();
	
	// Welding
	JsonObject welding = programs.add<JsonObject>();
	welding["id"] = PROGRAM_WELDING;
	welding["name"] = get_program_name(PROGRAM_WELDING);
	welding["description"] = get_program_description(PROGRAM_WELDING);
	
	// Heartbeat
	JsonObject heartbeat = programs.add<JsonObject>();
	heartbeat["id"] = PROGRAM_HEARTBEAT;
	heartbeat["name"] = get_program_name(PROGRAM_HEARTBEAT);
	heartbeat["description"] = get_program_description(PROGRAM_HEARTBEAT);

	// Breathing
	JsonObject breathing = programs.add<JsonObject>();
	breathing["id"] = PROGRAM_BREATHING;
	breathing["name"] = get_program_name(PROGRAM_BREATHING);
	breathing["description"] = get_program_description(PROGRAM_BREATHING);

	// Simple blink
	JsonObject simple_blink = programs.add<JsonObject>();
	simple_blink["id"] = PROGRAM_SIMPLE_BLINK;
	simple_blink["name"] = get_program_name(PROGRAM_SIMPLE_BLINK);
	simple_blink["description"] = get_program_description(PROGRAM_SIMPLE_BLINK);

	// TV flicler
	JsonObject tv_flicker = programs.add<JsonObject>();
	tv_flicker["id"] = PROGRAM_TV_FLICKER;
	tv_flicker["name"] = get_program_name(PROGRAM_TV_FLICKER);
	tv_flicker["description"] = get_program_description(PROGRAM_TV_FLICKER);

	// Firebox glow
	JsonObject firebox = programs.add<JsonObject>();
	firebox["id"] = PROGRAM_FIREBOX_GLOW;
	firebox["name"] = get_program_name(PROGRAM_FIREBOX_GLOW);
	firebox["description"] = get_program_description(PROGRAM_FIREBOX_GLOW);

	// Candle flicker
	JsonObject candle = programs.add<JsonObject>();
	candle["id"] = PROGRAM_CANDLE_FLICKER;
	candle["name"] = get_program_name(PROGRAM_CANDLE_FLICKER);
	candle["description"] = get_program_description(PROGRAM_CANDLE_FLICKER);

	// French crossing
	JsonObject french_crossing = programs.add<JsonObject>();
	french_crossing["id"] = PROGRAM_FRENCH_CROSSING;
	french_crossing["name"] = get_program_name(PROGRAM_FRENCH_CROSSING);
	french_crossing["description"] = get_program_description(PROGRAM_FRENCH_CROSSING);
	
	doc["total"] = programs.size();
	return doc;
}

JsonDocument ProgramManager::get_assigned_programs() {
	JsonDocument doc;
	JsonArray programs = doc["assigned_programs"].to<JsonArray>();
	
	if (module_manager) {
		for (uint8_t i = 0; i < module_manager->getModuleCount(); i++) {
			const PCA9685Module* module = module_manager->getModule(i);
			if (module) {
				for (uint8_t j = 0; j < module->getLedCount(); j++) {
					const LED* led = module->getLED(j);
					if (led && led->getProgramType() != PROGRAM_NONE) {
						JsonObject program = programs.add<JsonObject>();
						program["module_id"] = i;
						program["led_id"] = j;
						program["program_type"] = led->getProgramType();
						program["program_name"] = get_program_name(led->getProgramType());
						program["enabled"] = led->isEnabled();
					}
				}
			}
		}
	}
	
	doc["total"] = programs.size();
	return doc;
}

String ProgramManager::get_program_name(ProgramType type) {
	switch (type) {
		case PROGRAM_WELDING: return "Welding";
		case PROGRAM_HEARTBEAT: return "Heartbeat";
		case PROGRAM_BREATHING: return "Breathing";
		case PROGRAM_SIMPLE_BLINK: return "Simple Blink";
		case PROGRAM_TV_FLICKER: return "TV Flicker";
		case PROGRAM_FIREBOX_GLOW: return "Firebox Glow";
		case PROGRAM_CANDLE_FLICKER: return "Candle Flicker";
		case PROGRAM_FRENCH_CROSSING: return "French Level Crossing";
		default: return "None";
	}
}

String ProgramManager::get_program_description(ProgramType type) {
	switch (type) {
		case PROGRAM_WELDING: return "Simulates welding arc flashes with random intensity and timing";
		case PROGRAM_HEARTBEAT: return "Simulates a heartbeat rhythm with double pulse pattern";
		case PROGRAM_BREATHING: return "Simulates breathing";
		case PROGRAM_SIMPLE_BLINK: return "Simple 1 second on/off blinking pattern";
		case PROGRAM_TV_FLICKER: return "Television screen flickering with blue tint and random intensity changes";
		case PROGRAM_FIREBOX_GLOW: return "Wood fire simulation with crackling flames, ember pops and wind effects";
		case PROGRAM_CANDLE_FLICKER: return "Gentle candle or gas lamp flame flickering with organic variations";
		case PROGRAM_FRENCH_CROSSING: return "French railway level crossing light with realistic filament bulb behavior";
		default: return "No program";
	}
}

void ProgramManager::update_welding_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis) {
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) return;
	ProgramState* state = led_info->getProgramState();
	
	if (current_millis - state->last_update < 10) {
		return; // Update à 100Hz maximum
	}
	
	// Si aucun éclat n'est actif, vérifier s'il faut en déclencher un
	if (!state->active && current_millis >= state->next_event) {
		// Déclencher un nouvel éclat
		state->active = true;
		state->start_time = current_millis;
		
		// Intensité aléatoire pour cet éclat
		state->current_intensity = random(WELDING_MIN_INTENSITY, WELDING_MAX_INTENSITY + 1);
		
		// Programmer le prochain éclat
		unsigned long welding_duration = random(WELDING_MIN_DURATION, WELDING_MAX_DURATION + 1);
		unsigned long next_interval = random(WELDING_MIN_INTERVAL, WELDING_MAX_INTERVAL + 1);
		state->next_event = current_millis + welding_duration + next_interval;
		
		// Activer la LED immédiatement
		led_info->setBrightness(state->current_intensity);
		module_manager->applyLedBrightness(module_id, led_id);
	}
	
	// Si un éclat est actif, gérer son évolution
	if (state->active) {
		unsigned long elapsed_time = current_millis - state->start_time;
		unsigned long current_welding_duration = (WELDING_MIN_DURATION + WELDING_MAX_DURATION) / 2;
		
		if (elapsed_time < current_welding_duration * 0.7) {
			// Phase d'intensité maximale avec micro-variations
			uint16_t variation = random(-200, 201);
			uint16_t current_brightness = constrain(state->current_intensity + variation, 0, 4095);
			led_info->setBrightness(current_brightness);
			
		} else if (elapsed_time < current_welding_duration) {
			// Phase de fondu
			float fade_progress = (float)(elapsed_time - current_welding_duration * 0.7) / (current_welding_duration * 0.3);
			uint16_t current_brightness = state->current_intensity * (1.0 - fade_progress);
			led_info->setBrightness(current_brightness);
			
		} else {
			// Éclat terminé
			state->active = false;
			led_info->setBrightness(0);
		}
		
		module_manager->applyLedBrightness(module_id, led_id);
	}
	
	state->last_update = current_millis;
}

void ProgramManager::update_heartbeat_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis) {
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) return;
	ProgramState* state = led_info->getProgramState();
	
	if (current_millis - state->last_update < 20) {
		return; // Update à 50Hz pour fluidité
	}
	
	// Initialiser le cycle si nécessaire
	if (state->start_time == 0) {
		state->start_time = current_millis;
	}
	
	unsigned long cycle_time = (current_millis - state->start_time) % HEARTBEAT_CYCLE_DURATION;
	uint16_t target_brightness = 0;
	
	// Phase 1: Premier battement (systole)
	if (cycle_time < HEARTBEAT_BEAT1_DURATION) {
		target_brightness = HEARTBEAT_INTENSITY;
	}
	// Phase 2: Pause courte
	else if (cycle_time < HEARTBEAT_BEAT1_DURATION + HEARTBEAT_PAUSE1_DURATION) {
		target_brightness = 0;
	}
	// Phase 3: Second battement (diastole) - plus faible
	else if (cycle_time < HEARTBEAT_BEAT1_DURATION + HEARTBEAT_PAUSE1_DURATION + HEARTBEAT_BEAT2_DURATION) {
		target_brightness = HEARTBEAT_INTENSITY * 0.6; // 60% de l'intensité du premier
	}
	// Phase 4: Pause longue
	else {
		target_brightness = 0;
	}
	
	led_info->setBrightness(target_brightness);
	module_manager->applyLedBrightness(module_id, led_id);
	
	state->last_update = current_millis;
}

void ProgramManager::update_breathing_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis) {
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) return;
	ProgramState* state = led_info->getProgramState();
   
	if (current_millis - state->last_update < 20) {
		return; // Update à 50Hz pour fluidité
	}
   
	// Initialiser le cycle si nécessaire
	if (state->start_time == 0) {
		state->start_time = current_millis;
	}
   
	unsigned long cycle_time = (current_millis - state->start_time) % BREATHING_CYCLE_DURATION;
	uint16_t target_brightness = 0;
   
	// Phase 1: Inspiration (montée progressive)
	if (cycle_time < BREATHING_INHALE_DURATION) {
		float progress = (float)cycle_time / BREATHING_INHALE_DURATION;
		// Courbe sinusoïdale pour un effet plus naturel
		float sine_progress = sin(progress * PI / 2);
		target_brightness = BREATHING_MAX_INTENSITY * sine_progress;
	}
	// Phase 2: Rétention (maintien au maximum)
	else if (cycle_time < BREATHING_INHALE_DURATION + BREATHING_HOLD_DURATION) {
		target_brightness = BREATHING_MAX_INTENSITY;
	}
	// Phase 3: Expiration (descente progressive)
	else if (cycle_time < BREATHING_INHALE_DURATION + BREATHING_HOLD_DURATION + BREATHING_EXHALE_DURATION) {
		unsigned long exhale_time = cycle_time - BREATHING_INHALE_DURATION - BREATHING_HOLD_DURATION;
		float progress = (float)exhale_time / BREATHING_EXHALE_DURATION;
		// Courbe sinusoïdale inversée pour la descente
		float sine_progress = cos(progress * PI / 2);
		target_brightness = BREATHING_MAX_INTENSITY * sine_progress;
	}
	// Phase 4: Pause (minimum/éteint)
	else {
		target_brightness = BREATHING_MIN_INTENSITY; // Peut être 0 ou une valeur très faible
	}
   
	led_info->setBrightness(target_brightness);
	module_manager->applyLedBrightness(module_id, led_id);
   
	state->last_update = current_millis;
}

void ProgramManager::update_simple_blink_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis) {
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) return;
	ProgramState* state = led_info->getProgramState();
	
	if (current_millis - state->last_update < 50) {
		return; // Update every 50ms
	}
	
	// Initialize cycle if necessary
	if (state->start_time == 0) {
		state->start_time = current_millis;
	}
	
	unsigned long cycle_time = (current_millis - state->start_time) % (SIMPLE_BLINK_ON_DURATION + SIMPLE_BLINK_OFF_DURATION);
	uint16_t target_brightness = 0;
	
	// ON phase
	if (cycle_time < SIMPLE_BLINK_ON_DURATION) {
		target_brightness = SIMPLE_BLINK_INTENSITY;
	}
	// OFF phase
	else {
		target_brightness = 0;
	}
	
	led_info->setBrightness(target_brightness);
	module_manager->applyLedBrightness(module_id, led_id);
	
	state->last_update = current_millis;
}

void ProgramManager::update_tv_flicker_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis) {
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) return;
	ProgramState* state = led_info->getProgramState();
	
	if (current_millis - state->last_update < 20) {
		return; // Update at 50Hz for smooth flickering
	}
	
	// Check if it's time for next flicker change
	if (!state->active || current_millis >= state->next_event) {
		state->active = true;
		
		// Determine next change type
		uint8_t random_value = random(0, 100);
		
		if (random_value < TV_FLICKER_FLASH_PROBABILITY) {
			// Bright flash
			state->current_intensity = random(TV_FLICKER_MAX_INTENSITY * 0.8, TV_FLICKER_MAX_INTENSITY + 1);
		} else if (random_value < TV_FLICKER_FLASH_PROBABILITY + TV_FLICKER_DIM_PROBABILITY) {
			// Dim period
			state->current_intensity = random(TV_FLICKER_MIN_INTENSITY, TV_FLICKER_MIN_INTENSITY * 1.5);
		} else {
			// Normal variation around base intensity
			int16_t variation = random(-200, 201);
			state->current_intensity = constrain(TV_FLICKER_BASE_INTENSITY + variation, 
				TV_FLICKER_MIN_INTENSITY, 
				TV_FLICKER_MAX_INTENSITY);
		}
		
		// Schedule next change with random interval
		unsigned long next_interval = random(TV_FLICKER_MIN_INTERVAL, TV_FLICKER_MAX_INTERVAL + 1);
		state->next_event = current_millis + next_interval;
		
		// Add some micro-variations for more realism
		int16_t micro_variation = random(-50, 51);
		uint16_t final_intensity = constrain(state->current_intensity + micro_variation, 
			TV_FLICKER_MIN_INTENSITY, 
			TV_FLICKER_MAX_INTENSITY);
		
		led_info->setBrightness(final_intensity);
		module_manager->applyLedBrightness(module_id, led_id);
	}
	
	state->last_update = current_millis;
}

void ProgramManager::update_firebox_glow_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis) {
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) return;
	ProgramState* state = led_info->getProgramState();
	
	if (current_millis - state->last_update < 20) {
		return; // Update at 50Hz for realistic fire movement
	}
	
	// Initialize if needed
	if (state->start_time == 0) {
		state->start_time = current_millis;
		state->next_event = current_millis + random(FIREBOX_MIN_INTERVAL, FIREBOX_MAX_INTERVAL + 1);
		state->current_intensity = FIREBOX_BASE_INTENSITY;
		// Use parameters to track fire state: [0] = effect_type, [1] = effect_start_time
		state->parameters["effect_type"] = 0;
		state->parameters["effect_start_time"] = 0;
	}
	
	uint16_t target_intensity = FIREBOX_BASE_INTENSITY;
	uint8_t current_effect = state->parameters["effect_type"];
	unsigned long effect_start = state->parameters["effect_start_time"];
	
	// Handle active effects
	if (current_effect > 0) {
		unsigned long effect_duration = current_millis - effect_start;
		
		switch (current_effect) {
			case 1: // Ember pop
				if (effect_duration < FIREBOX_EMBER_DURATION) {
					// Quick bright flash then decay
					float progress = (float)effect_duration / FIREBOX_EMBER_DURATION;
					if (progress < 0.2) {
						// Sharp rise
						target_intensity = FIREBOX_BASE_INTENSITY + (1800 * (progress / 0.2));
					} else {
						// Quick decay
						target_intensity = FIREBOX_BASE_INTENSITY + (1800 * (1.0 - (progress - 0.2) / 0.8));
					}
				} else {
					// Effect finished
					state->parameters["effect_type"] = 0;
				}
				break;
				
			case 2: // Flame surge
				if (effect_duration < FIREBOX_SURGE_DURATION) {
					// Gradual rise and fall
					float progress = (float)effect_duration / FIREBOX_SURGE_DURATION;
					float surge_intensity;
					if (progress < 0.3) {
						// Rising
						surge_intensity = progress / 0.3;
					} else if (progress < 0.7) {
						// Sustained high
						surge_intensity = 1.0 + 0.2 * sin(progress * PI * 8); // Flickering at peak
					} else {
						// Falling
						surge_intensity = (1.0 - progress) / 0.3;
					}
					target_intensity = FIREBOX_BASE_INTENSITY + (1500 * surge_intensity);
				} else {
					// Effect finished
					state->parameters["effect_type"] = 0;
				}
				break;
				
			case 3: // Wind gust
				if (effect_duration < FIREBOX_WIND_DURATION) {
					// Irregular dancing flames
					float progress = (float)effect_duration / FIREBOX_WIND_DURATION;
					float wind_factor = sin(progress * PI * 3) * sin(progress * PI * 7) * sin(progress * PI * 11);
					wind_factor *= (1.0 - progress); // Decay over time
					target_intensity = FIREBOX_BASE_INTENSITY + (800 * wind_factor);
				} else {
					// Effect finished
					state->parameters["effect_type"] = 0;
				}
				break;
		}
	}
	
	// Check for new effects
	if (current_effect == 0 && current_millis >= state->next_event) {
		uint8_t random_value = random(0, 100);
		
		if (random_value < FIREBOX_EMBER_POP_PROBABILITY) {
			// Start ember pop
			state->parameters["effect_type"] = 1;
			state->parameters["effect_start_time"] = current_millis;
		} else if (random_value < FIREBOX_EMBER_POP_PROBABILITY + FIREBOX_FLAME_SURGE_PROBABILITY) {
			// Start flame surge
			state->parameters["effect_type"] = 2;
			state->parameters["effect_start_time"] = current_millis;
		} else if (random_value < FIREBOX_EMBER_POP_PROBABILITY + FIREBOX_FLAME_SURGE_PROBABILITY + FIREBOX_WIND_GUST_PROBABILITY) {
			// Start wind gust
			state->parameters["effect_type"] = 3;
			state->parameters["effect_start_time"] = current_millis;
		}
		
		// Schedule next potential event
		state->next_event = current_millis + random(FIREBOX_MIN_INTERVAL, FIREBOX_MAX_INTERVAL + 1);
	}
	
	// Add base fire variations (always active)
	if (current_effect == 0) {
		// Normal crackling variations
		int16_t base_variation = random(-400, 401);
		target_intensity = constrain(FIREBOX_BASE_INTENSITY + base_variation, 
									FIREBOX_MIN_INTENSITY, 
									FIREBOX_MAX_INTENSITY);
	}
	
	// Add micro-variations for organic feel
	int16_t micro_variation = random(-100, 101);
	target_intensity = constrain(target_intensity + micro_variation, 
								FIREBOX_MIN_INTENSITY, 
								FIREBOX_MAX_INTENSITY);
	
	// Smooth large transitions
	uint16_t current_brightness = led_info->getBrightness();
	int16_t intensity_diff = target_intensity - current_brightness;
	
	if (abs(intensity_diff) > 300 && current_effect != 1) { // Don't smooth ember pops
		// Large change - smooth it out
		if (intensity_diff > 0) {
			target_intensity = current_brightness + 150;
		} else {
			target_intensity = current_brightness - 150;
		}
	}
	
	led_info->setBrightness(target_intensity);
	module_manager->applyLedBrightness(module_id, led_id);
	
	state->last_update = current_millis;
}

void ProgramManager::update_candle_flicker_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis) {
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) return;
	ProgramState* state = led_info->getProgramState();
	
	if (current_millis - state->last_update < 25) {
		return; // Update at 40Hz for smooth organic movement
	}
	
	// Initialize if needed
	if (state->start_time == 0) {
		state->start_time = current_millis;
		state->next_event = current_millis + random(CANDLE_MIN_INTERVAL, CANDLE_MAX_INTERVAL + 1);
		state->current_intensity = CANDLE_BASE_INTENSITY;
	}
	
	// Check if it's time for a new flicker event
	if (current_millis >= state->next_event) {
		uint8_t random_value = random(0, 100);
		
		if (random_value < CANDLE_STRONG_FLICKER_PROBABILITY) {
			// Strong upward flicker
			state->current_intensity = random(CANDLE_MAX_INTENSITY * 0.9, CANDLE_MAX_INTENSITY + 1);
		} else if (random_value < CANDLE_STRONG_FLICKER_PROBABILITY + CANDLE_DIP_PROBABILITY) {
			// Gentle downward dip
			state->current_intensity = random(CANDLE_MIN_INTENSITY, CANDLE_MIN_INTENSITY * 1.2);
		} else {
			// Gentle variation around base
			int16_t gentle_variation = random(-150, 151);
			state->current_intensity = constrain(CANDLE_BASE_INTENSITY + gentle_variation, 
												CANDLE_MIN_INTENSITY, 
												CANDLE_MAX_INTENSITY);
		}
		
		// Schedule next flicker with variable timing
		unsigned long next_interval = random(CANDLE_MIN_INTERVAL, CANDLE_MAX_INTERVAL + 1);
		state->next_event = current_millis + next_interval;
	}
	
	// Add continuous subtle variations for organic feel
	int16_t subtle_variation = random(-30, 31);
	uint16_t target_intensity = constrain(state->current_intensity + subtle_variation, 
										 CANDLE_MIN_INTENSITY, 
										 CANDLE_MAX_INTENSITY);
	
	// Smooth transitions to avoid harsh changes
	uint16_t current_brightness = led_info->getBrightness();
	int16_t intensity_diff = target_intensity - current_brightness;
	
	if (abs(intensity_diff) > 200) {
		// Large change - smooth it out over multiple updates
		if (intensity_diff > 0) {
			target_intensity = current_brightness + 80;
		} else {
			target_intensity = current_brightness - 80;
		}
	} else if (abs(intensity_diff) > 50) {
		// Medium change - partial step
		target_intensity = current_brightness + (intensity_diff / 3);
	}
	
	led_info->setBrightness(target_intensity);
	module_manager->applyLedBrightness(module_id, led_id);
	
	state->last_update = current_millis;
}

void ProgramManager::update_french_crossing_program(uint8_t module_id, uint8_t led_id, unsigned long current_millis) {
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) return;
	ProgramState* state = led_info->getProgramState();
	
	if (current_millis - state->last_update < 10) {
		return; // Update at 100Hz for smooth filament effect
	}
	
	// Initialize cycle if necessary
	if (state->start_time == 0) {
		state->start_time = current_millis;
		// Use parameters to track filament state: [0] = phase_start_time, [1] = current_phase
		state->parameters["phase_start_time"] = current_millis;
		state->parameters["current_phase"] = 0; // 0 = OFF, 1 = ON
	}
	
	unsigned long cycle_time = (current_millis - state->start_time) % (FRENCH_CROSSING_ON_DURATION + FRENCH_CROSSING_OFF_DURATION);
	unsigned long phase_start = state->parameters["phase_start_time"];
	uint8_t current_phase = state->parameters["current_phase"];
	
	// Detect phase transitions
	bool should_be_on = (cycle_time < FRENCH_CROSSING_ON_DURATION);
	
	if ((should_be_on && current_phase == 0) || (!should_be_on && current_phase == 1)) {
		// Phase change detected
		state->parameters["phase_start_time"] = current_millis;
		state->parameters["current_phase"] = should_be_on ? 1 : 0;
		phase_start = current_millis;
		current_phase = should_be_on ? 1 : 0;
	}
	
	unsigned long phase_time = current_millis - phase_start;
	uint16_t target_brightness = 0;
	
	if (current_phase == 1) {
		// ON phase - filament heating up
		if (phase_time < FRENCH_CROSSING_WARMUP_DURATION) {
			// Filament warm-up: gradual increase with exponential curve
			float warmup_progress = (float)phase_time / FRENCH_CROSSING_WARMUP_DURATION;
			
			// Exponential heating curve: faster start, slower finish
			float exponential_progress = 1.0 - exp(-4.0 * warmup_progress);
			
			target_brightness = FRENCH_CROSSING_WARMUP_MIN + 
							   (FRENCH_CROSSING_MAX_INTENSITY - FRENCH_CROSSING_WARMUP_MIN) * exponential_progress;
		} else {
			// Full intensity with slight filament stability variations
			int16_t stability_variation = random(-25, 26);
			target_brightness = constrain(FRENCH_CROSSING_MAX_INTENSITY + stability_variation, 
										 FRENCH_CROSSING_MAX_INTENSITY - 50, 
										 FRENCH_CROSSING_MAX_INTENSITY);
		}
	} else {	
		if (phase_time < FRENCH_CROSSING_COOLDOWN_DURATION) {
			// Filament cool-down: exponential decay (slower than heating)
			float cooldown_progress = (float)phase_time / FRENCH_CROSSING_COOLDOWN_DURATION;
			
			// Exponential cooling curve: starts fast, then slower (thermal inertia)
			float exponential_decay = exp(-2.0 * cooldown_progress); // Slower decay than heating
			
			target_brightness = FRENCH_CROSSING_WARMUP_MIN * exponential_decay;
			
			// Add slight red glow persistence for realism
			//if (target_brightness < 50 && phase_time < FRENCH_CROSSING_COOLDOWN_DURATION * 0.8) {
			//	target_brightness += random(5, 15); // Faint residual glow
			//}
		} else {
			// Completely off
			target_brightness = 0;
		}
	}
	
	led_info->setBrightness(target_brightness);
	module_manager->applyLedBrightness(module_id, led_id);
	
	state->last_update = current_millis;
}

bool ProgramManager::initialize_led_state(uint8_t module_id, uint8_t led_id) {
	if (!module_manager || module_id >= module_manager->getModuleCount()) {
		return false;
	}
	
	const PCA9685Module* module = module_manager->getModule(module_id);
	if (!module || led_id >= module->getLedCount()) {
		return false;
	}
	
	LED* led_info = module_manager->getLED(module_id, led_id);
	if (!led_info) {
		return false;
	}

	switch (led_info->getProgramType()) {
		case PROGRAM_WELDING:
			initialize_welding_state(led_info->getProgramState());
			break;
		case PROGRAM_HEARTBEAT:
			initialize_default_state(led_info->getProgramState());
			break;
		case PROGRAM_BREATHING:
			initialize_default_state(led_info->getProgramState());
			break;
		case PROGRAM_SIMPLE_BLINK:
			initialize_default_state(led_info->getProgramState());
			break;
		case PROGRAM_TV_FLICKER:
			initialize_default_state(led_info->getProgramState());
			break;
		case PROGRAM_FIREBOX_GLOW:
			initialize_default_state(led_info->getProgramState());
			break;
		case PROGRAM_CANDLE_FLICKER:
			initialize_default_state(led_info->getProgramState());
			break;
		case PROGRAM_FRENCH_CROSSING:
			initialize_default_state(led_info->getProgramState());
			break;
		default:
			break;
	}

	return true;
}

void ProgramManager::initialize_default_state(ProgramState* state) {
	state->last_update = 0;
	state->start_time = millis();
	state->active = true;
	state->current_intensity = 0;
}

void ProgramManager::initialize_welding_state(ProgramState* state) {
	state->last_update = 0;
	state->next_event = millis() + random(1000, 3000); // Premier éclat dans 1-3 secondes
	state->active = false;
	state->start_time = 0;
	state->current_intensity = 0;
}
