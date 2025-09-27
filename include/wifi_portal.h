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
 * @file    wifi_portal.h
 * @brief   Declaration of the WiFiPortal class.
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <string>
#include <vector>
#include <functional>

/**
 * @brief WiFi configuration portal for ESP32
 * 
 * This class provides a captive portal interface for configuring WiFi credentials.
 * When activated, it creates an access point that users can connect to and configure
 * the WiFi settings through a web interface.
 */
class WiFiPortal {
	public:
		// === Public types, callbacks and enumerations ===

		/**
		 * @brief Configuration callback function type
		 * @param ssid The configured SSID
		 * @param password The configured password
		 * @return true if configuration was accepted, false otherwise
		 */
		using ConfigCallback = std::function<bool(const std::string& ssid, const std::string& password)>;

		/**
		 * @brief Portal configuration structure
		 */
		struct Config {
			std::string ap_ssid = "emfao-LightControl";            ///< Access point SSID
			std::string ap_password = "12345678";                  ///< Access point password (min 8 chars)
			std::string device_name = "emfao - Light Controller";  ///< Device name displayed in portal
			uint32_t portal_timeout_ms = 0;                        ///< Portal timeout (0 = no timeout)
			uint8_t max_connection_attempts = 2;                   ///< Max attempts before starting portal
			bool auto_connect = true;                              ///< Auto-connect after configuration
		};

		/**
		 * @brief Portal status enumeration
		 */
		enum class Status {
			IDLE,           ///< Portal is not active
			STARTING,       ///< Portal is starting up
			ACTIVE,         ///< Portal is active and waiting for configuration
			CONNECTING,     ///< Attempting to connect with new credentials
			SUCCESS,        ///< Configuration successful and connected
			FAILED,         ///< Configuration failed
			TIMEOUT         ///< Portal timed out
		};

	private:
		// === Private members ===

		Config config_;                           ///< Portal configuration
		ConfigCallback config_callback_;          ///< Configuration callback function
		Status status_;                           ///< Current portal status
		
		std::vector<std::string> scanned_networks_; ///< Last scanned networks
		std::string custom_template_;             ///< Custom HTML template
		
		unsigned long portal_start_time_;         ///< Time when portal was started
		bool initialized_;                        ///< Initialization status
		
		// Connection attempt tracking
		std::string pending_ssid_;                ///< SSID being tested
		std::string pending_password_;            ///< Password being tested
		unsigned long connection_start_time_;     ///< Start time of connection attempt
		static constexpr uint32_t CONNECTION_TIMEOUT_MS = 15000; ///< Connection timeout

	public:
		// === Constructor and Destructor ===

		/**
		 * @brief Default constructor
		 */
		WiFiPortal();

		/**
		 * @brief Destructor
		 */
		~WiFiPortal();

		/**
		 * @brief Delete copy constructor
		 */
		WiFiPortal(const WiFiPortal&) = delete;

		/**
		 * @brief Delete assignment operator
		 */
		WiFiPortal& operator=(const WiFiPortal&) = delete;

		// === Getters ===

		/**
		 * @brief Check if portal is currently active
		 * 
		 * @return true if portal is active, false otherwise
		 */
		bool isActive() const;

		/**
		 * @brief Get current portal status
		 * 
		 * @return Current portal status
		 */
		Status getStatus() const { return status_; }

		/**
		 * @brief Get status as string
		 * 
		 * @return Status as human-readable string
		 */
		std::string getStatusString() const;

		/**
		 * @brief Get scanned WiFi networks
		 * 
		 * @return Vector of WiFi network names
		 */
		std::vector<std::string> getScannedNetworks();

		// === Portal lifecycle ===

		/**
		 * @brief Initialize the portal with configuration
		 * 
		 * @param config Portal configuration
		 * @param callback Function to call when credentials are configured
		 * @return true if initialization successful, false otherwise
		 */
		bool initialize(const Config& config, ConfigCallback callback);

		/**
		 * @brief Start the configuration portal
		 * 
		 * This method starts the access point and web server for configuration.
		 * The portal will remain active until manually stopped.
		 * 
		 * @return true if portal started successfully, false otherwise
		 */
		bool startPortal();

		/**
		 * @brief Stop the configuration portal
		 * 
		 * Stops the access point and web server, returning to normal operation mode.
		 */
		void stopPortal();

		/**
		 * @brief Process portal operations
		 * 
		 * This method should be called regularly in the main loop to handle
		 * portal operations and DNS captive portal redirects.
		 */
		void handlePortal();

	private:
		// === Private functions ===

		/**
		 * @brief Check if portal has timed out
		 * 
		 * @return true if portal has timed out, false otherwise
		 */
		bool hasTimedOut() const;
};

/**
 * @brief Global WiFiPortal instance
 * 
 * This global instance provides access to the wifi portal
 * throughout the application.
 */
extern WiFiPortal wifi_portal;