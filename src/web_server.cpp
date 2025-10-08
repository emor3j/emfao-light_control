/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file web_server.cpp
 * @brief Implementation of WebServer class 
 * 
 * This file implements the WebServer class for ESP32 LED controller system.
 * 
 * See web_server.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include <Update.h>
#include <LittleFS.h>

#include "web_server.h"
#include "config.h"
#include "log.h"
#include "network.h"
#include "ota.h"
#include "pca9685.h"
#include "program.h"
#include "storage.h"
#include "wifi_portal.h"


/// Global web server instance
WebServer web_server(80);

// === Constructor and Destructor ===

// Default constructor
WebServer::WebServer(uint16_t port) : 
	server_(port),
	port_(port),
	server_running_(false),
	initialized_(false) {
	LOG_INFO("[WEBSERVER] WebServer instance created on port %u\n", port);
}

// Destructor
WebServer::~WebServer() {
	if (server_running_) {
		stop();
	}
	LOG_INFO("[WEBSERVER] WebServer instance destroyed\n");
}

// === Server Lifecycle Management ===

bool WebServer::initialize() {
	if (initialized_) {
		LOG_INFO("[WEBSERVER] Already initialized\n");
	return true;
	}
	
	LOG_INFO("[WEBSERVER] Initializing web server...\n");

	// Initialize LittleFS for static file serving
	if (!LittleFS.begin()) {
		LOG_ERROR("[WEBSERVER] Failed to mount LittleFS\n");
		return false;
	}
	LOG_INFO("[WEBSERVER] LittleFS mounted successfully\n");

	// Setup server components
	setupCorsHeaders();
	setupApiRoutes();
	setupStaticRoutes();
	setupErrorHandlers();

	initialized_ = true;
	LOG_INFO("[WEBSERVER] Web server initialized successfully\n");

	return true;
}

bool WebServer::start() {
	if (!initialized_) {
		LOG_ERROR("[WEBSERVER] Error: Cannot start - not initialized\n");
		return false;
	}
	
	if (server_running_) {
		LOG_INFO("[WEBSERVER] Server already running\n");
		return true;
	}

	server_.begin();
	server_running_ = true;
	
	LOG_INFO("[WEBSERVER] Web server started successfully\n");
	String port;
	if (port_ != 80) {
		port += ":";
		port += String(port_);
	}
	if (network_manager->isConnected()) {
		LOG_INFO("[WEBSERVER] Server available at: http://%s%s\n",
			network_manager->getIPAddress().c_str(),
			port);
	}
	if (network_manager->isPortalActive()) {
		LOG_INFO("[WEBSERVER] Server available at: http://%s%s\n",
			WiFi.softAPIP().toString().c_str(),
			port);
	}

	return true;
}

void WebServer::stop() {
	if (!server_running_) {
		return;
	}
	
	server_.end();
	server_running_ = false;
	LOG_INFO("[WEBSERVER] Web server stopped\n");
}

// === Private Setup Methods ===

void WebServer::setupCorsHeaders() {
	LOG_INFO("[WEBSERVER] Setting up CORS headers...\n");
	
	// Configure CORS headers for cross-origin requests
	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void WebServer::setupApiRoutes() {
	LOG_INFO("[WEBSERVER] Setting up API routes...\n");
	
	// System monitoring and health endpoints
	server_.on("/api/health", HTTP_GET, createHealthHandler());
	server_.on("/api/system", HTTP_GET, createSystemHandler());
	
	// Hardware management endpoints
	server_.on("/api/modules", HTTP_GET, createModulesHandler());
	server_.on("/api/leds", HTTP_GET, createLedsHandler());
	server_.on("/api/leds", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, createUpdateLedHandler());

	// Program management endpoints
	server_.on("/api/programs", HTTP_GET, createProgramsHandler());

	// OTA update endpoints
	server_.on("/api/ota/status", HTTP_GET, createOtaStatusHandler());
	server_.on("/api/ota/upload", HTTP_POST, 
		[](AsyncWebServerRequest *request){
			// Called when upload is complete - send final response
			JsonDocument doc;
			doc["success"] = Update.hasError() ? false : true;
			doc["error"] = Update.hasError() ? Update.errorString() : "";
			
			String response;
			serializeJson(doc, response);
			request->send(Update.hasError() ? 500 : 200, "application/json", response);
			
			// Restart system after successful update
			if (!Update.hasError()) {
				LOG_ERROR("[WEBSERVER] OTA update completed successfully, restarting...\n");
				delay(1000);
				ESP.restart();
			}
		}, 
		createOtaUploadHandler()
	);

	// System logs endpoints
	server_.on("/api/logs", HTTP_GET, createLogsHandler());
	server_.on("/api/logs", HTTP_DELETE, createClearLogsHandler());

	// Configuration endpoints
	// WiFi configuration endpoints
	server_.on("/api/wifi/scan", HTTP_GET, createWifiScanHandler());
	server_.on("/api/wifi/status", HTTP_GET, createWifiStatusHandler());
	server_.on("/api/wifi/config", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, createWifiConfigHandler());

	// Save/Load endpoints
	server_.on("/api/save", HTTP_GET, createSaveHandler());
	server_.on("/api/load", HTTP_GET, createLoadHandler());
}

void WebServer::setupStaticRoutes() {
	LOG_INFO("[WEBSERVER] Setting up static file routes...\n");
	
	// Main web interface
	server_.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/index.html", "text/html");
	});

	// Sytem logs
	server_.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/logs.html", "text/html");
	});
	
	// Static assets with proper MIME types
	server_.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/style.css", "text/css");
	});

	server_.on("/js/logs_refresh.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/js/logs_refresh.js", "application/javascript");
	});

	server_.on("/js/logs_download.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/js/logs_download.js", "application/javascript");
	});
	
	server_.on("/js/upload.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/js/upload.js", "application/javascript");
	});

	server_.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/upload.html", "text/html");
	});

	// Configuration page
	server_.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/config.html", "text/html");
	});

	server_.on("/js/config.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/js/config.js", "application/javascript");
	});

	// === Routes pour détection automatique des captive portals ===

	// Android - teste cette URL pour détecter les captive portals  
	server_.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
		LOG_INFO("[WEBSERVER] Android captive portal detection\n");
		// Android s'attend à un code 204, on redirige à la place
		request->redirect("http://192.168.4.1/");
	});

	// iOS et macOS
	server_.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
		LOG_INFO("[WEBSERVER] iOS captive portal detection\n");
		request->redirect("http://192.168.4.1/");
	});

	// Autre test Android
	server_.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {
		LOG_INFO("[WEBSERVER] Android gen_204 detection\n");
		request->redirect("http://192.168.4.1/");
	});

	// Microsoft Windows
	server_.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
		LOG_INFO("[WEBSERVER] Windows captive portal detection\n");
		request->send(200, "text/plain", "Microsoft NCSI");
	});
}

void WebServer::setupErrorHandlers() {
	LOG_INFO("[WEBSERVER] Setting up error handlers...\n");
	
	
	
	server_.onNotFound([](AsyncWebServerRequest *request) {
		// Code 200 for all OPTIONS request
		if (request->method() == HTTP_OPTIONS) {
			request->send(200);
		} else {
			// Code 404
			if (request->url().startsWith("/api/")) {
				request->send(404, "application/json", "{\"error\":\"API endpoint not found\"}");
			} else {
				request->send(LittleFS, "/404.html", "text/html");
			}
		}
	});
}

// === API Handler Implementations ===

void WebServer::handleGetHealth(AsyncWebServerRequest *request) {
	JsonDocument doc;
	
	String overall_status = "healthy";
	
	// Memory health assessment
	bool memory_ok = ESP.getFreeHeap() > 10000;        // Warning threshold
	bool memory_critical = ESP.getFreeHeap() < 5000;   // Critical threshold
	
	// Module health assessment
	uint8_t initialized_modules = module_manager ? module_manager->getInitializedModuleCount() : 0;
	uint8_t total_modules = module_manager ? module_manager->getModuleCount() : 0;
	bool modules_ok = (initialized_modules == total_modules) && (total_modules > 0);
	
	// Determine overall system status
	if (memory_critical) {
		overall_status = "critical";
	} else if (!modules_ok || !memory_ok) {
		overall_status = "degraded";
	}
	
	// Build response document
	doc["status"] = overall_status;
	doc["timestamp"] = millis();
	doc["uptime_ms"] = millis();
	
	JsonObject checks = doc["checks"].to<JsonObject>();
	checks["modules"] = modules_ok;
	checks["memory"] = memory_ok;
	
	JsonObject metrics = doc["metrics"].to<JsonObject>();
	metrics["free_heap_kb"] = ESP.getFreeHeap() / 1024;
	metrics["modules_ready"] = String(initialized_modules) + "/" + String(total_modules);
	
	// Return appropriate HTTP status code
	int http_status = (overall_status == "critical") ? 503 : 200;
	
	String response;
	serializeJson(doc, response);
	request->send(http_status, "application/json", response);
}

void WebServer::handleGetSystem(AsyncWebServerRequest *request) {
	JsonDocument doc;
	
	doc["timestamp"] = millis();
	
	// Detailed system information
	JsonObject system = doc["system"].to<JsonObject>();
	system["uptime_ms"] = millis();
	
	// Formatted uptime calculation
	unsigned long uptime_seconds = millis() / 1000;
	unsigned long hours = uptime_seconds / 3600;
	unsigned long minutes = (uptime_seconds % 3600) / 60;
	unsigned long seconds = uptime_seconds % 60;
	system["uptime_formatted"] = String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
	
	// Memory information
	JsonObject memory = system["memory"].to<JsonObject>();
	memory["free_heap"] = ESP.getFreeHeap();
	memory["total_heap"] = ESP.getHeapSize();
	memory["free_heap_kb"] = ESP.getFreeHeap() / 1024;
	memory["total_heap_kb"] = ESP.getHeapSize() / 1024;
	memory["usage_percent"] = ((ESP.getHeapSize() - ESP.getFreeHeap()) * 100) / ESP.getHeapSize();
	memory["free_psram"] = ESP.getFreePsram();
	memory["total_psram"] = ESP.getPsramSize();
	memory["free_psram_kb"] = ESP.getFreePsram() / 1024;
	memory["total_psram_kb"] = ESP.getPsramSize() / 1024;
	
	// CPU information
	JsonObject cpu = system["cpu"].to<JsonObject>();
	cpu["freq_mhz"] = ESP.getCpuFreqMHz();
	cpu["cores"] = ESP.getChipCores();
	cpu["temperature_c"] = temperatureRead();
	
	// Chip information
	JsonObject chip = system["chip"].to<JsonObject>();
	chip["model"] = ESP.getChipModel();
	chip["revision"] = ESP.getChipRevision();
	chip["sdk_version"] = ESP.getSdkVersion();
	chip["idf_version"] = esp_get_idf_version();
	
	// Flash information
	JsonObject flash = system["flash"].to<JsonObject>();
	flash["size_mb"] = ESP.getFlashChipSize() / 1024 / 1024;
	flash["speed_mhz"] = ESP.getFlashChipSpeed() / 1000000;
	String flash_mode;
	switch (ESP.getFlashChipMode()) {
		case FM_QIO: flash_mode = "QIO"; break;
		case FM_QOUT: flash_mode = "QOUT"; break;
		case FM_DIO: flash_mode = "DIO"; break;
		case FM_DOUT: flash_mode = "DOUT"; break;
		default: flash_mode = "Unknown"; break;
	}
	flash["mode"] = flash_mode;
	
	// Detailed WiFi status
	JsonObject wifi = doc["wifi"].to<JsonObject>();
	wifi["mac_address"] = WiFi.macAddress();
	wifi["ip_address"] = WiFi.localIP().toString();
	wifi["rssi_dbm"] = WiFi.RSSI();
	wifi["ssid"] = WiFi.SSID();
	wifi["gateway"] = WiFi.gatewayIP().toString();
	wifi["subnet"] = WiFi.subnetMask().toString();
	wifi["dns"] = WiFi.dnsIP().toString();
	
	// I2C configuration
	JsonObject i2c = doc["i2c"].to<JsonObject>();
	i2c["sda_pin"] = config.getI2cSdaPin();
	i2c["scl_pin"] = config.getI2cSclPin();
	i2c["clock_hz"] = 100000;
	i2c["addr_min"] = "0x" + String(config.getPca9685AddrMin(), HEX);
	i2c["addr_max"] = "0x" + String(config.getPca9685AddrMax(), HEX);
	
	// Module summary (without details)
	JsonObject modules_summary = doc["modules_summary"].to<JsonObject>();
	if (module_manager) {
		modules_summary["detected_count"] = module_manager->getModuleCount();
		modules_summary["initialized_count"] = module_manager->getInitializedModuleCount();
	} else {
		modules_summary["detected_count"] = 0;
		modules_summary["initialized_count"] = 0;
	}
	modules_summary["max_modules"] = config.getPca9685ModuleMax();
	
	// LED summary (without details)
	JsonObject leds_summary = doc["leds_summary"].to<JsonObject>();
	if (module_manager) {
		leds_summary["total_count"] = module_manager->getTotalLedCount();
		leds_summary["enabled_count"] = module_manager->getEnabledLedCount();
	} else {
		leds_summary["total_count"] = 0;
		leds_summary["enabled_count"] = 0;
	}
	leds_summary["max_per_module"] = config.getPca9685LedMax();
	
	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

void WebServer::handleGetModules(AsyncWebServerRequest *request) {
	JsonDocument doc;
	JsonArray pca9685 = doc["pca9685"].to<JsonArray>();

	if (module_manager) {
		for (uint8_t i = 0; i < module_manager->getModuleCount(); i++) {
			const PCA9685Module* module = module_manager->getModule(i);
			if (module) {
				JsonObject module_obj = pca9685.add<JsonObject>();
				module_obj["id"] = i;
				module_obj["address"] = "0x" + String(module->getAddress(), HEX);
				module_obj["name"] = module->getName();
				module_obj["detected"] = module->isDetected();
				module_obj["initialized"] = module->isInitialized();
				module_obj["led_count"] = module->getLedCount();
			}
		}
		
		doc["total_modules"] = module_manager->getModuleCount();
		doc["total_leds"] = module_manager->getTotalLedCount();
	} else {
		doc["total_modules"] = 0;
		doc["total_leds"] = 0;
	}

	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

void WebServer::handleGetLeds(AsyncWebServerRequest *request) {
	JsonDocument doc;
	JsonArray leds = doc["leds"].to<JsonArray>();

	if (module_manager) {
		for (uint8_t i = 0; i < module_manager->getModuleCount(); i++) {
			const PCA9685Module* module = module_manager->getModule(i);
			if (module) {
				for (uint8_t j = 0; j < module->getLedCount(); j++) {
					const LED* led = module->getLED(j);
					if (led) {
						JsonObject led_obj = leds.add<JsonObject>();
						led_obj["module_id"] = i;
						led_obj["led_id"] = j;
						led_obj["name"] = led->getName();
						led_obj["enabled"] = led->isEnabled();
						led_obj["brightness"] = led->getBrightness();
						led_obj["program_type"] = led->getProgramType();
						led_obj["program_name"] = program_manager->get_program_name(led->getProgramType());
						led_obj["is_controlled_by_program"] = (led->getProgramType() != PROGRAM_NONE);
					}
				}
			}
		}
		
		doc["total_modules"] = module_manager->getModuleCount();
		doc["total_leds"] = module_manager->getTotalLedCount();
	} else {
		doc["total_modules"] = 0;
		doc["total_leds"] = 0;
	}

	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

void WebServer::handleUpdateLed(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
	JsonDocument doc;
	deserializeJson(doc, (char*)data);
	
	uint8_t module = doc["module"];
	uint8_t led = doc["led"];
	
	// Validate module index
	if (!module_manager || module >= module_manager->getModuleCount()) {
		request->send(400, "application/json", "{\"error\":\"Invalid module index\"}");
		return;
	}

	// Validate LED index within module
	const PCA9685Module* module_ptr = module_manager->getModule(module);
	if (!module_ptr || led >= module_ptr->getLedCount()) {
		request->send(400, "application/json", "{\"error\":\"Invalid LED index\"}");
		return;
	}

	LED* led_info = module_manager->getLED(module, led);
	if (!led_info) {
		request->send(400, "application/json", "{\"error\":\"LED not found\"}");
		return;
	}
	
	// Process individual property updates
	if (!doc["name"].isNull()) {
		led_info->setName(doc["name"].as<String>());
	}
	
	// Handle enable/disable state changes
	if (!doc["enabled"].isNull()) {
		led_info->setEnabled(doc["enabled"]);
		if (!led_info->isEnabled()) {
			led_info->setBrightness(0);
			module_manager->applyLedBrightness(module, led);
		} else {
			// Restore brightness
			module_manager->applyLedBrightness(module, led);
		}
	}

	// Handle program assignment changes
	if (!doc["program_type"].isNull()) {
		ProgramType new_program = (ProgramType)doc["program_type"].as<int>();
		
		if (new_program == PROGRAM_NONE) {
			program_manager->unassign_program(module, led);
		} else {
			program_manager->assign_program(module, led, new_program);
		}
	}

	// Handle brightness updates (only when not program-controlled)
	if (!doc["brightness"].isNull()) {
		led_info->setBrightness(doc["brightness"]);
		if (led_info->getProgramType() == PROGRAM_NONE && led_info->isEnabled()) {
			module_manager->applyLedBrightness(module, led);
		}
	}
	
	// Build comprehensive response
	JsonDocument response_doc;
	response_doc["success"] = true;
	response_doc["led_info"]["module_id"] = module;
	response_doc["led_info"]["led_id"] = led;
	response_doc["led_info"]["name"] = led_info->getName();
	response_doc["led_info"]["enabled"] = led_info->isEnabled();
	response_doc["led_info"]["brightness"] = led_info->getBrightness();
	response_doc["led_info"]["program_type"] = led_info->getProgramType();
	response_doc["led_info"]["program_name"] = program_manager->get_program_name(led_info->getProgramType());
	response_doc["led_info"]["is_controlled_by_program"] = (led_info->getProgramType() != PROGRAM_NONE);
	
	String response_str;
	serializeJson(response_doc, response_str);
	
	request->send(200, "application/json", response_str);
}

void WebServer::handleGetPrograms(AsyncWebServerRequest *request) {
	JsonDocument doc;
	
	// Available programs
	JsonDocument available = program_manager->get_available_programs();
	doc["available_programs"] = available["programs"];
	
	// Assigned programs
	JsonDocument assigned = program_manager->get_assigned_programs();
	doc["assigned_programs"] = assigned["assigned_programs"];
	
	// Statistics
	doc["stats"]["total_available"] = available["total"];
	doc["stats"]["total_assigned"] = assigned["total"];
	doc["timestamp"] = millis();
	
	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

void WebServer::handleOtaStatus(AsyncWebServerRequest *request) {
	JsonDocument doc;
	
	doc["ota_active"] = ota_manager->isActive();
	doc["ota_status"] = ota_manager->getStatusString();
	doc["ota_updating"] = ota_manager->isUpdating();
	doc["ota_progress"] = ota_manager->getProgress();
	doc["ota_hostname"] = ota_manager->getHostname();
	doc["ota_port"] = ota_manager->getPort();
	doc["last_error"] = ota_manager->getLastError();
	
	// System health check for OTA
	bool memory_ok = ESP.getFreeHeap() > 50000; // Need enough memory for OTA
	doc["memory_sufficient"] = memory_ok;
	doc["wifi_connected"] = WiFi.isConnected();
	doc["wifi_rssi"] = WiFi.RSSI();
	doc["ready_for_ota"] = memory_ok && WiFi.isConnected() && ota_manager->isActive();
	
	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

void WebServer::handleOtaUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
	static bool ota_started = false;
	static size_t total_size = 0;
	static unsigned long start_time = 0;
	
	// First chunk - initialize OTA
	if (index == 0) {
		ota_started = false;
		total_size = request->contentLength();
		start_time = millis();
		
		LOG_INFO("[WEBSERVER] Starting OTA update: %s (%zu bytes)\n", filename.c_str(), total_size);
		
		// Check available space
		size_t free_space = ESP.getFreeSketchSpace();
		if (total_size > free_space) {
			LOG_INFO("[WEBSERVER] Error: Not enough space. Need: %zu, Available: %zu\n", total_size, free_space);
			request->send(507, "application/json", "{\"success\":false,\"error\":\"Insufficient storage space\"}");
			return;
		}
		
		// Start update
		if (!Update.begin(total_size)) {
			LOG_ERROR("[WEBSERVER] Cannot start update: %s\n", Update.errorString());
			request->send(500, "application/json", "{\"success\":false,\"error\":\"Cannot start update\"}");
			return;
		}
		
		ota_started = true;
		LOG_INFO("[WEBSERVER] OTA update started successfully\n");
	}
	
	// Write data chunk
	if (ota_started && len > 0) {
		size_t written = Update.write(data, len);
		if (written != len) {
			LOG_ERROR("[WEBSERVER] Error: Write failed. Expected: %zu, Written: %zu\n", len, written);
			Update.abort();
			request->send(500, "application/json", "{\"success\":false,\"error\":\"Write failed\"}");
			return;
		}
		
		// Progress logging (every 10%)
		static uint8_t last_progress = 0;
		uint8_t current_progress = (index + len) * 100 / total_size;
		if (current_progress >= last_progress + 10) {
			unsigned long elapsed = millis() - start_time;
			size_t speed = elapsed > 0 ? ((index + len) * 1000) / elapsed : 0;
			LOG_INFO("[WEBSERVER] OTA Progress: %u%% (%zu/%zu bytes) Speed: %zu B/s\n", 
				current_progress, index + len, total_size, speed);
			last_progress = current_progress;
		}
	}
	
	// Final chunk - complete update
	if (final) {
		if (ota_started) {
			if (Update.end(true)) {
				unsigned long elapsed = millis() - start_time;
				size_t avg_speed = elapsed > 0 ? (total_size * 1000) / elapsed : 0;
				LOG_INFO("[WEBSERVER] OTA update completed successfully in %lu ms (avg: %zu B/s)\n", 
						elapsed, avg_speed);
				// Response will be sent by the POST handler
			} else {
				LOG_ERROR("[WEBSERVER] Error: Update end failed: %s\n", Update.errorString());
				// Response will be sent by the POST handler with error
			}
		} else {
			LOG_ERROR("[WEBSERVER] OTA was not properly started\n");
		}
		ota_started = false;
	}
}

void WebServer::handleGetLogs(AsyncWebServerRequest *request) {
	// Parse query parameters
	int count = 0;
	unsigned long since_timestamp = 0;
	
	if (request->hasParam("count")) {
		count = request->getParam("count")->value().toInt();
	}
	
	if (request->hasParam("since")) {
		since_timestamp = request->getParam("since")->value().toInt();
	}
	
	// Get logs from LogManager
	std::vector<LogEntry> logs;
	if (since_timestamp > 0) {
		logs = LogManager::getInstance().getLogsSince(since_timestamp);
	} else if (count > 0) {
		logs = LogManager::getInstance().getRecentLogs(count);
	} else {
		logs = LogManager::getInstance().getLogs();
	}
	
	// Build JSON response
	JsonDocument doc;
	JsonArray logs_array = doc["logs"].to<JsonArray>();
	
	for (const auto& entry : logs) {
		JsonObject log_obj = logs_array.add<JsonObject>();
		log_obj["timestamp"] = entry.timestamp;
		log_obj["level"] = static_cast<int>(entry.level);
		log_obj["message"] = entry.message;
	}
	
	// Add buffer statistics
	JsonObject stats = doc["stats"].to<JsonObject>();
	size_t total_entries;
	uint8_t buffer_utilization;
	LogManager::getInstance().getBufferStats(total_entries, buffer_utilization);
	
	stats["total_entries"] = total_entries;
	stats["buffer_utilization"] = buffer_utilization;
	
	doc["timestamp"] = millis();
	doc["count"] = logs.size();
	
	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

void WebServer::handleClearLogs(AsyncWebServerRequest *request) {
	LogManager::getInstance().clearLogs();
	
	JsonDocument doc;
	doc["success"] = true;
	doc["message"] = "All logs cleared successfully";
	doc["timestamp"] = millis();
	
	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

// Config handlers

void WebServer::handleConfigPage(AsyncWebServerRequest *request) {
	request->send(LittleFS, "/config.html", "text/html");
}

void WebServer::handleWifiScan(AsyncWebServerRequest *request) {
	static bool scan_in_progress = false;
	static unsigned long last_scan_start = 0;
	
	// Check if scan is already in progress
	if (scan_in_progress) {
		int16_t scan_result = WiFi.scanComplete();
		
		if (scan_result == WIFI_SCAN_RUNNING) {
			// Scan still running
			JsonDocument doc;
			doc["scanning"] = true;
			doc["message"] = "Scan in progress...";
			
			String response;
			serializeJson(doc, response);
			request->send(200, "application/json", response);
			return;
		}
		
		// Scan completed
		scan_in_progress = false;
		
		JsonDocument doc;
		JsonArray networks = doc["networks"].to<JsonArray>();
		
		if (scan_result > 0) {
			LOG_INFO("[WEBSERVER] WiFi scan completed: %d networks found\n", scan_result);
			
			for (int i = 0; i < scan_result; i++) {
				JsonObject network = networks.add<JsonObject>();
				network["ssid"] = WiFi.SSID(i);
				network["rssi"] = WiFi.RSSI(i);
				network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
			}
			
			WiFi.scanDelete();
			doc["count"] = scan_result;
		} else {
			LOG_WARNING("[WEBSERVER] WiFi scan failed or no networks found\n");
			doc["count"] = 0;
			doc["error"] = (scan_result == WIFI_SCAN_FAILED) ? "Scan failed" : "No networks found";
		}
		
		doc["scanning"] = false;
		doc["timestamp"] = millis();
		
		String response;
		serializeJson(doc, response);
		request->send(200, "application/json", response);
		
	} else {
		// Start new async scan
		LOG_INFO("[WEBSERVER] Starting async WiFi scan...\n");
		
		// Clear any previous scan results
		WiFi.scanDelete();
		
		// Start async scan
		if (WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING) {
			scan_in_progress = true;
			last_scan_start = millis();
			
			JsonDocument doc;
			doc["scanning"] = true;
			doc["message"] = "Scan started...";
			
			String response;
			serializeJson(doc, response);
			request->send(200, "application/json", response);
		} else {
			LOG_ERROR("[WEBSERVER] Failed to start WiFi scan\n");
			request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to start scan\"}");
		}
	}
}

void WebServer::handleWifiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
	JsonDocument doc;
	deserializeJson(doc, (char*)data);
	
	if (!doc["ssid"].is<String>() || !doc["password"].is<String>()) {
		request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing SSID or password\"}");
		return;
	}
	
	std::string ssid = doc["ssid"].as<String>().c_str();
	std::string password = doc["password"].as<String>().c_str();
	
	// Save credentials to storage
	bool saved = storage_manager->save_wifi_credentials(ssid, password);
	
	JsonDocument response_doc;
	response_doc["success"] = saved;
	
	if (saved) {
		response_doc["message"] = "WiFi credentials saved. System will reboot in 3 seconds...";
		response_doc["reboot"] = true;
	} else {
		response_doc["error"] = "Failed to save credentials";
	}
	
	String response_str;
	serializeJson(response_doc, response_str);
	request->send(saved ? 200 : 500, "application/json", response_str);
	
	// Reboot after short delay if saved successfully
	if (saved) {
		LOG_INFO("[WEBSERVER] WiFi credentials saved, rebooting in 3 seconds...\n");
		delay(3000);
		ESP.restart();
	}
}

void WebServer::handleWifiStatus(AsyncWebServerRequest *request) {
	JsonDocument doc;
	
	if (network_manager) {
		doc["connected"] = network_manager->isConnected();
		doc["ssid"] = network_manager->getCurrentSSID();
		doc["ip_address"] = network_manager->getIPAddress();
		doc["signal_strength"] = network_manager->getSignalStrength();
		doc["mac_address"] = network_manager->getMACAddress();
	} else {
		doc["connected"] = false;
	}
	
	// Check if credentials are stored
	doc["credentials_stored"] = storage_manager->has_wifi_credentials();
	
	doc["timestamp"] = millis();
	
	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

void WebServer::handleSave(AsyncWebServerRequest *request) {
	JsonDocument doc;
	doc["saved"] = storage_manager->save_configuration();

	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

void WebServer::handleLoad(AsyncWebServerRequest *request) {
	JsonDocument doc;
	doc["loaded"] = storage_manager->load_configuration();

	String response;
	serializeJson(doc, response);
	request->send(200, "application/json", response);
}

// === Lambda Wrapper Methods ===

std::function<void(AsyncWebServerRequest*)> WebServer::createHealthHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleGetHealth(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createSystemHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleGetSystem(request);
	};
}

std::function<void(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t)> WebServer::createUpdateLedHandler() {
	return [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
		this->handleUpdateLed(request, data, len, index, total);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createModulesHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleGetModules(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createLedsHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleGetLeds(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createProgramsHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleGetPrograms(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createOtaStatusHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleOtaStatus(request);
	};
}

std::function<void(AsyncWebServerRequest*, String, size_t, uint8_t*, size_t, bool)> WebServer::createOtaUploadHandler() {
	return [this](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
		this->handleOtaUpload(request, filename, index, data, len, final);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createLogsHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleGetLogs(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createClearLogsHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleClearLogs(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createConfigPageHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleConfigPage(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createWifiScanHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleWifiScan(request);
	};
}

std::function<void(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t)> WebServer::createWifiConfigHandler() {
	return [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
		this->handleWifiConfig(request, data, len, index, total);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createWifiStatusHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleWifiStatus(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createSaveHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleSave(request);
	};
}

std::function<void(AsyncWebServerRequest*)> WebServer::createLoadHandler() {
	return [this](AsyncWebServerRequest* request) {
		this->handleLoad(request);
	};
}