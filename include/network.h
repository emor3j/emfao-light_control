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
 * @file    network.h
 * @brief   Declaration of the NetworkManager class.
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <string>
#include <functional>
#include <WiFi.h>


/**
 * @brief Network management class for WiFi connection handling
 * 
 * This class provides WiFi connection management functionality including
 * initial connection setup, connection monitoring, automatic reconnection,
 * and configuration portal support for credential management.
 */
class NetworkManager {
	public:
		// === Public structures ===

		/**
		 * @brief WiFi credential structure
		 */
		struct Credentials {
			std::string ssid;        ///< WiFi network SSID
			std::string password;    ///< WiFi network password
			
			/**
			 * @brief Check if credentials are valid (not empty)
			 * @return true if both SSID and password are not empty
			 */
			bool isValid() const {
				return !ssid.empty() && !password.empty();
			}
		};

		/**
		 * @brief Credentials update callback function type
		 * @param credentials New WiFi credentials to save
		 * @return true if credentials were saved successfully
		 */
		using CredentialsCallback = std::function<bool(const Credentials& credentials)>;

	private:
		// === Private members ===

		Credentials credentials_;                 ///< Current WiFi credentials
		CredentialsCallback credentials_callback_; ///< Callback for credential updates
		
		bool is_connected_;                       ///< Current connection status
		bool was_connected_;                      ///< Previous connection status for change detection
		unsigned long last_connection_check_;     ///< Last time connection was checked (millis)
		bool initialized_;                        ///< Whether the manager has been initialized
		
		// Portal mode settings
		uint8_t max_connection_attempts_;         ///< Max attempts before starting portal
		uint8_t current_attempt_count_;           ///< Current connection attempt counter
		bool portal_started_;                     ///< Whether portal has been started

	public:
		// === Public functions ===

		// === Constructor and Destructor ===

		/**
		 * @brief Default constructor
		 */
		NetworkManager();
		
		/**
		 * @brief Destructor - ensures proper cleanup
		 */
		~NetworkManager();
		
		/**
		 * @brief Delete copy constructor (singleton-like behavior)
		 */
		NetworkManager(const NetworkManager&) = delete;
		
		/**
		 * @brief Delete assignment operator (singleton-like behavior)
		 */
		NetworkManager& operator=(const NetworkManager&) = delete;

		// === Getters ===

		/**
		 * @brief Get current IP address
		 * 
		 * @return IP address, empty string  or if not connected
		 */
		std::string getIPAddress() const;
		
		/**
		 * @brief Get WiFi signal strength
		 * 
		 * @return RSSI value in dBm, 0 if not connected
		 */
		int32_t getSignalStrength() const;
		
		/**
		 * @brief Get MAC address
		 * 
		 * @return MAC address as string
		 */
		std::string getMACAddress() const;
		
		/**
		 * @brief Get current SSID
		 * 
		 * @return SSID as string, empty if not connected
		 */
		std::string getCurrentSSID() const;

		/**
		 * @brief Get current stored credentials
		 * 
		 * @return Current WiFi credentials
		 */
		Credentials getCredentials() const { return credentials_; }

		// === Other functions ===
		
		/**
		 * @brief Initialize WiFi connection with credentials
		 * 
		 * @param credentials WiFi network credentials
		 * @param timeout_seconds Maximum time to wait for connection (default: 30)
		 * @return true if connection successful, false otherwise
		 */
		bool initialize(const Credentials& credentials, int timeout_seconds = 30);
		
		/**
		 * @brief Initialize WiFi connection with credentials and callback
		 * 
		 * @param credentials WiFi network credentials
		 * @param callback Callback function for credential updates
		 * @param timeout_seconds Maximum time to wait for connection (default: 30)
		 * @return true if connection successful, false otherwise
		 */
		bool initialize(const Credentials& credentials, CredentialsCallback callback, int timeout_seconds = 30);
		
		/**
		 * @brief Check current WiFi connection status and attempt reconnection if needed
		 * 
		 * This method should be called periodically to monitor connection health
		 * and automatically reconnect if the connection is lost. It will start the
		 * configuration portal if enabled and needed.
		 * 
		 * @param reconnect_timeout_seconds Maximum time to wait for reconnection (default: 10)
		 * @return true if connected, false otherwise
		 */
		bool checkConnection(int reconnect_timeout_seconds = 10);
		
		/**
		 * @brief Get current connection status
		 * 
		 * @return true if WiFi is connected, false otherwise
		 */
		bool isConnected() const;

		/**
		 * @brief Disconnect from WiFi network
		 */
		void disconnect();
		
		/**
		 * @brief Update WiFi credentials
		 * 
		 * This method updates the stored credentials and attempts to connect
		 * with the new credentials. If callback is set, credentials will be saved.
		 * 
		 * @param credentials New WiFi credentials
		 * @return true if connection with new credentials successful
		 */
		bool updateCredentials(const Credentials& credentials);
		
		/**
		 * @brief Force start configuration portal
		 * 
		 * Manually start the configuration portal. Portal will remain active
		 * until manually stopped, regardless of connection status.
		 * 
		 * @return true if portal started successfully
		 */
		bool startConfigurationPortal();
		
		/**
		 * @brief Stop configuration portal
		 * 
		 * Manually stop the configuration portal and return to normal operation.
		 */
		void stopConfigurationPortal();
		
		/**
		 * @brief Check if configuration portal is active
		 * 
		 * @return true if portal is currently active
		 */
		bool isPortalActive() const;
		
		/**
		 * @brief Print connection status and details to Serial
		 */
		void printStatus() const;

	private:
		// === Private functions ===

		/**
		 * @brief Attempt to connect to WiFi with stored credentials
		 * 
		 * @param timeout_seconds Maximum time to wait for connection
		 * @return true if connection successful, false otherwise
		 */
		bool attemptConnection(int timeout_seconds);
		
		/**
		 * @brief Log connection status change
		 * 
		 * @param connected New connection status
		 */
		void logConnectionChange(bool connected);
		
		/**
		 * @brief Handle portal configuration callback
		 * 
		 * @param ssid Configured SSID
		 * @param password Configured password
		 * @return true if configuration accepted
		 */
		bool handlePortalConfiguration(const std::string& ssid, const std::string& password);
};

/**
 * @brief Global network manager instance
 * 
 * This global instance provides access to the wifi connection
 * throughout the application. It should be initialized before
 * setting up web server.
 */
extern std::unique_ptr<NetworkManager> network_manager;