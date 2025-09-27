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
 * @brief   Declaration of CaptiveDNSServer class
 * 
 * This file declares the CaptiveDNSServer class for ESP32 LED controller system.
 * This DNS server redirects all DNS queries to the access point IP for captive portal functionality.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-24
 */

#pragma once

#include <DNSServer.h>
#include <WiFi.h>
#include <memory>
#include <string>

/**
 * @brief Captive DNS server for WiFi configuration portal
 * 
 * This class provides DNS redirection functionality for captive portal operation.
 * It redirects all DNS queries to the access point IP address, forcing web browsers
 * to display the configuration portal when users try to access any website.
 */
class CaptiveDNSServer {
	public:
		// === Public structures ===

		/**
		 * @brief DNS server configuration structure
		 */
		struct Config {
			uint16_t port;                         ///< DNS server port (standard is 53)
			std::string redirect_domain;           ///< Domain to redirect (wildcard for all)
			uint32_t ttl;                          ///< DNS response TTL in seconds
			
			/**
			 * @brief Default constructor with standard values
			 */
			Config() : port(53), redirect_domain("*"), ttl(60) {}
		};

		/**
		 * @brief DNS server status enumeration
		 */
		enum class Status {
			IDLE,           ///< Server is not running
			STARTING,       ///< Server is starting
			ACTIVE,         ///< Server is active and processing requests
			ERROR           ///< Server encountered an error
		};

	private:
		// === Private members ===

		Config config_;                           ///< DNS server configuration
		Status status_;                           ///< Current server status
		std::unique_ptr<DNSServer> dns_server_;   ///< Underlying DNS server instance
		IPAddress redirect_ip_;                   ///< IP address to redirect to
		unsigned long start_time_;                ///< Time when server was started
		unsigned long last_request_time_;         ///< Time of last DNS request
		uint32_t request_count_;                  ///< Total number of requests processed

	public:
		// === Constructor and Destructor ===

		/**
		 * @brief Default constructor
		 */
		CaptiveDNSServer();

		/**
		 * @brief Destructor
		 */
		~CaptiveDNSServer();

		/**
		 * @brief Delete copy constructor
		 */
		CaptiveDNSServer(const CaptiveDNSServer&) = delete;

		/**
		 * @brief Delete assignment operator
		 */
		CaptiveDNSServer& operator=(const CaptiveDNSServer&) = delete;

		// === Getters ===

		/**
		 * @brief Check if DNS server is active
		 * 
		 * @return true if server is running and processing requests
		 */
		bool isActive() const { return status_ == Status::ACTIVE; }

		/**
		 * @brief Get current server status
		 * 
		 * @return Current DNS server status
		 */
		Status getStatus() const { return status_; }

		/**
		 * @brief Get status as string
		 * 
		 * @return Status as human-readable string
		 */
		std::string getStatusString() const;

		/**
		 * @brief Get redirect IP address
		 * 
		 * @return IP address that DNS queries are redirected to
		 */
		IPAddress getRedirectIP() const { return redirect_ip_; }

		/**
		 * @brief Get total request count
		 * 
		 * @return Number of DNS requests processed since start
		 */
		uint32_t getRequestCount() const { return request_count_; }

		/**
		 * @brief Get server uptime
		 * 
		 * @return Server uptime in milliseconds, 0 if not active
		 */
		unsigned long getUptime() const;

		// === Server lifecycle ===

		/**
		 * @brief Initialize DNS server with configuration
		 * 
		 * @param config DNS server configuration
		 * @return true if initialization successful
		 */
		bool initialize(const Config& config = Config());

		/**
		 * @brief Start DNS server
		 * 
		 * Starts the DNS server and begins redirecting all DNS queries
		 * to the access point IP address. The AP must be active before
		 * starting the DNS server.
		 * 
		 * @return true if server started successfully
		 */
		bool start();

		/**
		 * @brief Stop DNS server
		 * 
		 * Stops the DNS server and cleans up resources.
		 */
		void stop();

		/**
		 * @brief Process DNS requests
		 * 
		 * This method should be called regularly in the main loop to
		 * process incoming DNS requests and maintain server statistics.
		 */
		void handleRequests();

		/**
		 * @brief Print server status and statistics
		 */
		void printStatus() const;

	private:
		// === Private functions ===

		/**
		 * @brief Update internal statistics
		 */
		void updateStats();
};

/**
 * @brief Global CaptiveDNSServer instance
 * 
 * This global instance provides access to the captive DNS server
 * throughout the application.
 */
extern std::unique_ptr<CaptiveDNSServer> captive_dns_server;