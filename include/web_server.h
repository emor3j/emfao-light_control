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
 * @file    web_server.h
 * @brief   Declaration of the WebServer class.
 * 
 * This file implements the WebServer class that provides a comprehensive
 * web server and REST API system for the LED controller. The implementation
 * uses object-oriented design principles with proper encapsulation and
 * error handling throughout.
 * 
 * The web server architecture includes:
 * - AsyncWebServer for high-performance concurrent request handling
 * - LittleFS integration for static file serving
 * - JSON-based REST API with comprehensive error handling
 * - CORS support for cross-origin web applications
 * - Chunked upload support for large firmware files
 * - Real-time system status and health monitoring
 * 
 * Class Design:
 * - Singleton pattern for global access
 * - RAII principle for resource management
 * - Lambda wrappers for AsyncWebServer callback compatibility
 * - Comprehensive error handling and logging
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#pragma once

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

/**
 * @class WebServer
 * @brief HTTP web server and REST API manager for LED controller
 * 
 * The WebServer class provides a comprehensive web interface for the LED
 * controller system. It combines static file serving with a full REST API
 * to enable both web-based user interaction and programmatic control.
 * 
 * Key Features:
 * - Asynchronous request handling for high performance
 * - JSON-based REST API with comprehensive error handling
 * - Static file serving from LittleFS filesystem
 * - CORS support for cross-origin web applications
 * - Chunked upload support for firmware updates
 * - Real-time system monitoring and health checks
 * 
 * The class follows the singleton pattern as there should be only one
 * web server instance per system. All methods are designed to be
 * thread-safe for use with the AsyncWebServer library.
 */
class WebServer {
	private:
		// === Private Members ===
		
		AsyncWebServer server_;		///< AsyncWebServer instance for handling HTTP requests
		uint16_t port_;			///< TCP port number for HTTP server
		bool server_running_;		///< Flag indicating if server is currently running
		bool initialized_;		///< Flag indicating if server has been initialized

	public:
		// === Constructor and Destructor ===
		
		/**
		 * @brief Construct WebServer instance
		 * 
		 * Creates a new WebServer instance with the specified port.
		 * The server is not started automatically - call initialize()
		 * and start() to begin accepting connections.
		 * 
		 * @param port TCP port number for HTTP server (default: 81)
		 */
		explicit WebServer(uint16_t port = 80);
		
		/**
		 * @brief Destructor
		 * 
		 * Cleans up server resources and stops all connections.
		 * The destructor will automatically call stop() if the
		 * server is still running.
		 */
		~WebServer();
		
		// === Server Lifecycle Management ===
		
		/**
		 * @brief Initialize the web server
		 * 
		 * Prepares the web server for operation by:
		 * - Initializing LittleFS filesystem for static files
		 * - Setting up CORS headers for cross-origin requests
		 * - Configuring all API endpoints and route handlers
		 * - Setting up error handlers for 404 and other HTTP errors
		 * - Preparing OTA upload handling
		 * 
		 * This method must be called before start() and after all
		 * system components (modules, LEDs, programs) are initialized.
		 * 
		 * @return true if initialization successful, false on error
		 * 
		 * @note LittleFS must be available and properly formatted
		 */
		bool initialize();
		
		/**
		 * @brief Start the web server
		 * 
		 * Begins accepting HTTP connections on the configured port.
		 * The server will immediately start processing requests
		 * after this method completes successfully.
		 * 
		 * @return true if server started successfully, false on error
		 * 
		 * @note initialize() must be called successfully before start()
		 */
		bool start();
		
		/**
		 * @brief Stop the web server
		 * 
		 * Stops accepting new connections and closes existing ones.
		 * This method is safe to call multiple times and can be used
		 * to temporarily disable the web interface.
		 * 
		 * @note Existing connections may take time to close gracefully
		 */
		void stop();
		
		/**
		 * @brief Check if server is currently running
		 * 
		 * @return true if server is accepting connections, false otherwise
		 */
		bool isRunning() const { return server_running_; }
		
		/**
		 * @brief Get the configured server port
		 * 
		 * @return TCP port number the server is bound to
		 */
		uint16_t getPort() const { return port_; }
		
	private:
		// === Server Setup Methods ===
		
		/**
		 * @brief Configure CORS headers
		 * 
		 * Sets up Cross-Origin Resource Sharing headers to allow
		 * web applications from different domains to access the API.
		 */
		void setupCorsHeaders();
		
		/**
		 * @brief Configure API endpoints
		 * 
		 * Sets up all REST API routes including system monitoring,
		 * hardware management, and program control endpoints.
		 */
		void setupApiRoutes();
		
		/**
		 * @brief Configure static file routes
		 * 
		 * Sets up routes for serving static web interface files
		 * from the LittleFS filesystem with proper MIME types.
		 */
		void setupStaticRoutes();
		
		/**
		 * @brief Configure error handlers
		 * 
		 * Sets up custom error handlers for 404 Not Found and
		 * other HTTP error conditions with appropriate responses.
		 */
		void setupErrorHandlers();
		
		// === System Status and Health API Handlers ===
		
		/**
		 * @brief Handle system health check requests
		 * 
		 * Endpoint: GET /api/health
		 * 
		 * Evaluates system health by checking memory, modules, and other
		 * critical resources. Returns HTTP 503 for critical conditions.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleGetHealth(AsyncWebServerRequest *request);
		
		/**
		 * @brief Handle detailed system information requests
		 * 
		 * Endpoint: GET /api/system
		 * 
		 * Provides comprehensive system information including hardware
		 * details, network status, and performance metrics.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleGetSystem(AsyncWebServerRequest *request);
		
		// === Hardware Management API Handlers ===
		
		/**
		 * @brief Handle PCA9685 module information requests
		 * 
		 * Endpoint: GET /api/modules
		 * 
		 * Returns information about all detected PCA9685 modules including
		 * addresses, names, detection status, and LED counts.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleGetModules(AsyncWebServerRequest *request);
		
		/**
		 * @brief Handle LED status information requests
		 * 
		 * Endpoint: GET /api/leds
		 * 
		 * Returns detailed information about all LEDs across all modules
		 * including current state and program assignments.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleGetLeds(AsyncWebServerRequest *request);
		
		/**
		 * @brief Handle LED configuration update requests
		 * 
		 * Endpoint: POST /api/leds
		 * Content-Type: application/json
		 * 
		 * Updates LED configuration including brightness, enable state,
		 * and program assignments through a unified interface.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 * @param data Pointer to JSON request body data
		 * @param len Length of the request body data
		 * @param index Current chunk index for large uploads
		 * @param total Total size of the request body
		 */
		void handleUpdateLed(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
		
		// === Program Management API Handlers ===
		
		/**
		 * @brief Handle program information requests
		 * 
		 * Endpoint: GET /api/programs
		 * 
		 * Returns information about available programs and current
		 * program assignments across all LEDs.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleGetPrograms(AsyncWebServerRequest *request);
		
		// === OTA (Over-The-Air) Update API Handlers ===
		
		/**
		 * @brief Handle OTA system status requests
		 * 
		 * Endpoint: GET /api/ota/status
		 * 
		 * Provides detailed OTA system status including readiness
		 * checks and current operation state.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleOtaStatus(AsyncWebServerRequest *request);
		
		/**
		 * @brief Handle OTA firmware upload requests
		 * 
		 * Endpoint: POST /api/ota/upload
		 * Content-Type: application/octet-stream
		 * 
		 * Processes firmware uploads with validation, progress monitoring,
		 * and automatic system restart on successful completion.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 * @param filename Name of the uploaded firmware file
		 * @param index Current chunk offset in bytes
		 * @param data Pointer to chunk data buffer
		 * @param len Length of current chunk
		 * @param final True if this is the last chunk of the upload
		 * 
		 * @warning System automatically restarts after successful update
		 */
		void handleOtaUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

		/**
		 * @brief Handle system logs requests
		 * 
		 * Endpoint: GET /api/logs
		 * Query parameters:
		 * - count: number of recent entries to return (optional)
		 * - since: timestamp to get logs since (optional)
		 * 
		 * Returns JSON array of log entries with timestamps and messages.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleGetLogs(AsyncWebServerRequest *request);

		/**
		 * @brief Handle log clearing requests
		 * 
		 * Endpoint: DELETE /api/logs
		 * 
		 * Clears all stored log entries from the buffer.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleClearLogs(AsyncWebServerRequest *request);

		// === Configuration handlers ===

		/**
		 * @brief Handle WiFi configuration page request
		 * 
		 * Endpoint: GET /config
		 * 
		 * Serves the WiFi configuration page for system setup.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleConfigPage(AsyncWebServerRequest *request);

		/**
		 * @brief Handle WiFi scan request
		 * 
		 * Endpoint: GET /api/wifi/scan
		 * 
		 * Returns list of available WiFi networks.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleWifiScan(AsyncWebServerRequest *request);

		/**
		 * @brief Handle WiFi configuration update
		 * 
		 * Endpoint: POST /api/wifi/config
		 * Content-Type: application/json
		 * 
		 * Updates WiFi configuration with new credentials.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 * @param data Pointer to JSON request body data
		 * @param len Length of the request body data
		 * @param index Current chunk index for large uploads
		 * @param total Total size of the request body
		 */
		void handleWifiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

		/**
		 * @brief Handle WiFi status request
		 * 
		 * Endpoint: GET /api/wifi/status
		 * 
		 * Returns current WiFi connection status and network information.
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleWifiStatus(AsyncWebServerRequest *request);

		/**
		 * @brief Save current configuration 
		 * 
		 * Endpoint: GET /api/save
		 * 
		 * Returns save operation status
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleSave(AsyncWebServerRequest *request);

		/**
		 * @brief Load saved configuration 
		 * 
		 * Endpoint: GET /api/load
		 * 
		 * Returns save operation status
		 * 
		 * @param request AsyncWebServerRequest object containing HTTP request details
		 */
		void handleLoad(AsyncWebServerRequest *request);
		
		// === Static Lambda Wrapper Methods ===
		// These methods provide static context for AsyncWebServer callbacks
		
		/**
		 * @brief Create lambda wrapper for health endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createHealthHandler();
		
		/**
		 * @brief Create lambda wrapper for system info endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createSystemHandler();
		
		/**
		 * @brief Create lambda wrapper for modules endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createModulesHandler();
		
		/**
		 * @brief Create lambda wrapper for LEDs endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createLedsHandler();
		
		/**
		 * @brief Create lambda wrapper for LED update endpoint
		 * @return Lambda function compatible with AsyncWebServer body handler
		 */
		std::function<void(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t)> createUpdateLedHandler();
		
		/**
		 * @brief Create lambda wrapper for programs endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createProgramsHandler();
		
		/**
		 * @brief Create lambda wrapper for OTA status endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createOtaStatusHandler();
		
		/**
		 * @brief Create lambda wrapper for OTA upload endpoint
		 * @return Lambda function compatible with AsyncWebServer upload handler
		 */
		std::function<void(AsyncWebServerRequest*, String, size_t, uint8_t*, size_t, bool)> createOtaUploadHandler();

		/**
		 * @brief Create lambda wrapper for logs endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createLogsHandler();

		/**
		 * @brief Create lambda wrapper for clear logs endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createClearLogsHandler();

		/**
		 * @brief Create lambda wrapper for config page endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createConfigPageHandler();

		/**
		 * @brief Create lambda wrapper for WiFi scan endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createWifiScanHandler();

		/**
		 * @brief Create lambda wrapper for WiFi config endpoint
		 * @return Lambda function compatible with AsyncWebServer body handler
		 */
		std::function<void(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t)> createWifiConfigHandler();

		/**
		 * @brief Create lambda wrapper for WiFi status endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createWifiStatusHandler();

		/**
		 * @brief Create lambda wrapper for save endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createSaveHandler();

		/**
		 * @brief Create lambda wrapper for load endpoint
		 * @return Lambda function compatible with AsyncWebServer
		 */
		std::function<void(AsyncWebServerRequest*)> createLoadHandler();
};

/**
 * @brief Global WebServer instance
 * 
 * This global instance provides access to the web server
 * throughout the application. It should be initialized after
 * WiFi connection and OTA initialisation in the setup() function.
 */
extern WebServer web_server;