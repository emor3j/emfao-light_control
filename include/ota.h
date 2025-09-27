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
 * @file    ota.h
 * @brief   Declaration of the OTAManager class.
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <functional>
#include <string>


/**
 * @brief Wrapper for ESP32 OTA functionality
 * 
 * This class provides OTA (Over-The-Air) update system with 
 * error handling and progress tracking.
 */
class OTAManager {
	public:
		// === Public enumerations and structures ===

		/**
		 * @brief OTA update status enumeration
		 */
		enum class Status : uint8_t {
			IDLE,		///< OTA not active
			INITIALIZING,	///< OTA service starting
			READY,		///< OTA service ready for updates
			UPDATING,	///< Update in progress
			SUCCESS,	///< Update completed successfully
			FAILED,		///< Update failed
			REBOOTING	///< System rebooting after update
		};

		/**
		 * @brief OTA progress callback function type
		 * @param progress Current progress (0-100)
		 * @param total Total bytes to transfer
		 * @param current Current bytes transferred
		 */
		using ProgressCallback = std::function<void(uint8_t progress, size_t total, size_t current)>;

		/**
		 * @brief OTA status callback function type
		 * @param status Current OTA status
		 * @param message Optional status message
		 */
		using StatusCallback = std::function<void(Status status, const std::string& message)>;

		/**
		 * @brief OTA configuration structure
		 */
		struct Config {
			std::string hostname;	///< OTA hostname
			std::string password;	///< OTA password (empty = no auth)
			uint16_t port;		///< OTA port
			bool auto_reboot;	///< Auto reboot after successful update
			uint32_t timeout_ms;	///< OTA operation timeout
			bool enable_mdns;	///< Enable mDNS for discovery
			
			// Constructor with default values
			Config() : 
				hostname("emfao-led-controller"),
				password(""),
				port(3232),
				auto_reboot(true),
				timeout_ms(30000),
				enable_mdns(true) {}
		};

	private:
		// === Private members ===

		Config config_;				///< OTA configuration
		Status status_;				///< Current status
		std::string last_error_;		///< Last error message
		
		ProgressCallback progress_callback_;	///< Progress update callback
		StatusCallback status_callback_;	///< Status change callback
		
		uint8_t current_progress_;		///< Current progress percentage
		size_t total_size_;			///< Total update size
		size_t current_size_;			///< Current transferred size
		
		unsigned long start_time_;		///< Update start time
		unsigned long last_progress_time_;	///< Last progress update time
		
		bool initialized_;			///< Initialization status
		bool active_;				///< Service active status

	public:
		// === Public methods ===

		// === Constructor and Destructor ===

		/**
		 * @brief Default constructor
		 */
		OTAManager();

		/**
		 * @brief Destructor
		 */
		~OTAManager();

		/**
		 * @brief Delete copy constructor
		 */
		OTAManager(const OTAManager&) = delete;

		/**
		 * @brief Delete assignment operator
		 */
		OTAManager& operator=(const OTAManager&) = delete;

		// === Getters ===

		/**
		 * @brief Check if OTA service is active
		 * 
		 * @return true if OTA service is running
		 */
		bool isActive() const { return active_; }

		/**
		 * @brief Get current OTA status
		 * 
		 * @return Current status
		 */
		Status getStatus() const { return status_; }

		/**
		 * @brief Get status as string
		 * 
		 * @return Status as human-readable string
		 */
		std::string getStatusString() const;

		/**
		 * @brief Get current progress percentage
		 * 
		 * @return Progress (0-100), or 0 if not updating
		 */
		uint8_t getProgress() const { return current_progress_; }

		/**
		 * @brief Get OTA hostname
		 * 
		 * @return Configured hostname
		 */
		std::string getHostname() const { return config_.hostname; }

		/**
		 * @brief Get OTA port
		 * 
		 * @return Configured port
		 */
		uint16_t getPort() const { return config_.port; }

		/**
		 * @brief Check if update is in progress
		 * 
		 * @return true if update is currently running
		 */
		bool isUpdating() const;

		/**
		 * @brief Get last error message
		 * 
		 * @return Last error message, empty if no error
		 */
		std::string getLastError() const { return last_error_; }

		// === Setters ===

		/**
		 * @brief Set progress callback
		 * 
		 * @param callback Function to call on progress updates
		 */
		void setProgressCallback(ProgressCallback callback) { progress_callback_ = callback; }

		/**
		 * @brief Set status callback
		 * 
		 * @param callback Function to call on status changes
		 */
		void setStatusCallback(StatusCallback callback) { status_callback_ = callback; }

		// === Other functions ===

		/**
		 * @brief Initialize OTA service
		 * 
		 * @param config OTA configuration
		 * @return true if initialization successful
		 */
		bool initialize(const Config& config = Config{});

		/**
		 * @brief Start OTA service
		 * 
		 * @return true if service started successfully
		 */
		bool start();

		/**
		 * @brief Stop OTA service
		 */
		void stop();

		/**
		 * @brief Handle OTA operations (call in main loop)
		 * 
		 * This method should be called regularly to process OTA requests
		 * and maintain connection stability.
		 */
		void handle();

	private:
		// === Private methods ===

		/**
		 * @brief Setup OTA event handlers
		 */
		void setupEventHandlers();

		/**
		 * @brief Handle OTA start event
		 */
		void handleStart();

		/**
		 * @brief Handle OTA end event
		 */
		void handleEnd();

		/**
		 * @brief Handle OTA progress event
		 * 
		 * @param progress Current progress
		 * @param total Total size
		 */
		void handleProgress(unsigned int progress, unsigned int total);

		/**
		 * @brief Handle OTA error event
		 * 
		 * @param error Error code
		 */
		void handleError(ota_error_t error);

		/**
		 * @brief Convert error code to string
		 * 
		 * @param error OTA error code
		 * @return Error description
		 */
		std::string errorToString(ota_error_t error) const;

		/**
		 * @brief Update status and notify callbacks
		 * 
		 * @param new_status New status
		 * @param message Optional message
		 */
		void updateStatus(Status new_status, const std::string& message = "");

		/**
		 * @brief Check WiFi connection stability
		 * 
		 * @return true if WiFi is stable for OTA
		 */
		bool isWiFiStable() const;
};

/**
 * @brief Global OTA manager instance
 * 
 * This global instance provides access to the OTA manager
 * throughout the application. It should be initialized after WiFi
 * connection in the setup() function.
 */
extern std::unique_ptr<OTAManager> ota_manager;