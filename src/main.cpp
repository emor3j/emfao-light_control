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
 * @file    main.cpp
 * @brief   Main loop.
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include <WiFi.h>
#include <Wire.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <string>

#include "config.h"
#include "dns_server.h"
#include "network.h"
#include "wifi_portal.h"
#include "ota.h"
#include "storage.h"
#include "pca9685.h"
#include "web_server.h"
#include "program.h"
#include "log.h"


PCA9685Module* pca_modules;
uint8_t detected_modules_count;

// Default WiFi credentials (will be overridden by stored credentials)
//NetworkManager::Credentials default_credentials = {"default_ssid", "default_password"};

// Function declarations
void setup_i2c();
void print_system_info();

void setup_i2c() {
	LOG_INFO("[I2CBUS] Setting up I2C...\n");
	
	Wire.begin(config.getI2cSdaPin(), config.getI2cSclPin());
	Wire.setClock(100000); // 100kHz for reliable communication
	
	LOG_INFO("[I2CBUS] I2C initialized - SDA: %d, SCL: %d\n", config.getI2cSdaPin(), config.getI2cSclPin());
}

void print_system_info() {
	// Update uptime
	unsigned long uptime_seconds = millis() / 1000;
	unsigned long hours = uptime_seconds / 3600;
	unsigned long minutes = (uptime_seconds % 3600) / 60;
	unsigned long seconds = uptime_seconds % 60;
	String uptime = String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";

	// Update memory
	String memory = "Free: " + String(ESP.getFreeHeap() / 1024) + " KB";
	memory += " | Total: " + String(ESP.getHeapSize() / 1024) + " KB";

	// Update temperature
	String temp = String(temperatureRead(), 1) + " °C";

	LOG_INFO("[SYSTEM] Uptime: %s\n", uptime.c_str());
	LOG_INFO("[SYSTEM] Memory: %s\n", memory.c_str());
	LOG_INFO("[SYSTEM] Temp: %s\n", temp.c_str());
}

void setup() {
	// Initialize serial port
	Serial.begin(115200);
	
	// Wait serial port
	while (!Serial) {
		delay(10);
	}

	// Initialize random generator
  	randomSeed(analogRead(0));
	
	// Délai pour laisser le temps au moniteur série de se connecter
	delay(1000);

	// Initialize LogManager
	if (!LogManager::getInstance().initialize(50)) {
		LOG_ERROR("[MAIN] Warning: LogManager initialization failed\n");
	}
	
	LOG_INFO("=== INFORMATIONS SYSTÈME ESP32 ===\n");
	
	// ===== INFORMATIONS CHIP =====
	LOG_INFO("--- INFORMATIONS CHIP ---\n");
	
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	
	LOG_INFO("Modèle du chip: %s\n", ESP.getChipModel());
	LOG_INFO("Révision du chip: %d\n", ESP.getChipRevision());
	LOG_INFO("Nombre de cœurs: %d\n", ESP.getChipCores());
	
	String features = "Fonctionnalités: ";
	if (chip_info.features & CHIP_FEATURE_WIFI_BGN) features += "WiFi ";
	if (chip_info.features & CHIP_FEATURE_BT) features += "Bluetooth ";
	if (chip_info.features & CHIP_FEATURE_BLE) features += "Bluetooth LE ";
	if (chip_info.features & CHIP_FEATURE_EMB_FLASH) features += "FLASH ";
	if (chip_info.features & CHIP_FEATURE_EMB_PSRAM) features += "PSRAM ";
	if (chip_info.features & CHIP_FEATURE_IEEE802154) features += "IEEE802154 ";
	LOG_INFO("%s\n", features.c_str());
	
	// ===== INFORMATIONS MÉMOIRE =====
	LOG_INFO("--- INFORMATIONS MÉMOIRE ---\n");
	LOG_INFO("Taille de la flash: %d MB\n", ESP.getFlashChipSize() / 1024 / 1024);
	LOG_INFO("Mémoire heap libre: %d KB\n", ESP.getFreeHeap() / 1024);
	LOG_INFO("Taille totale du heap: %d KB\n", ESP.getHeapSize() / 1024);
	LOG_INFO("Mémoire PSRAM libre: %d KB\n", ESP.getFreePsram() / 1024);
	LOG_INFO("Taille totale PSRAM: %d KB\n", ESP.getPsramSize() / 1024);
	
	// ===== INFORMATIONS FRÉQUENCES =====
	LOG_INFO("--- INFORMATIONS FRÉQUENCES ---\n");
	LOG_INFO("Fréquence CPU: %d MHz\n", ESP.getCpuFreqMHz());
	LOG_INFO("Fréquence flash: %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
	
	String flash_mode = "Mode flash: ";
	switch (ESP.getFlashChipMode()) {
		case FM_QIO:
			flash_mode += "QIO";
			break;
		case FM_QOUT:
			flash_mode += "QOUT";
			break;
		case FM_DIO:
			flash_mode += "DIO";
			break;
		case FM_DOUT:
			flash_mode += "DOUT";
			break;
		default:
			flash_mode += "Inconnu";
	}
	LOG_INFO("%s\n", flash_mode.c_str());
	
	// ===== INFORMATIONS RÉSEAU =====
	LOG_INFO("--- INFORMATIONS RÉSEAU ---\n");
	LOG_INFO("Adresse MAC WiFi: %s\n", WiFi.macAddress().c_str());
	
	uint8_t baseMac[6];
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	LOG_INFO("Adresse MAC base: %02X:%02X:%02X:%02X:%02X:%02X\n", 
		baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
	
	// ===== INFORMATIONS SDK ET SYSTÈME =====
	LOG_INFO("--- INFORMATIONS SDK ET SYSTÈME ---\n");
	LOG_INFO("Version SDK: %s\n", ESP.getSdkVersion());
	LOG_INFO("Version IDF: %s\n", esp_get_idf_version());
	LOG_INFO("Temps de fonctionnement: %d secondes\n", millis() / 1000);
	
	// ===== INFORMATIONS SUPPLÉMENTAIRES =====
	LOG_INFO("--- INFORMATIONS SUPPLÉMENTAIRES ---\n");
	LOG_INFO("Température interne: %.2f °C\n", temperatureRead());
	LOG_INFO("Voltage d'alimentation: %.2f V\n", analogReadMilliVolts(A0) * 2 / 1000.0);
	
	LOG_INFO("=== FIN DES INFORMATIONS ===\n");

	// ===== CONFIGURATION =====
	config.printConfiguration();

	// Setup I2C bus
	setup_i2c();

	// Initialize storage manager
	program_manager.reset(new ProgramManager());
	if (storage_manager->initialize()) {
		LOG_INFO("[MAIN] Storage manager initialized successfully\n");
	} else {
		LOG_ERROR("[MAIN] Storage manager initialization failed\n");
	}

	// Setup PCA9685 modules
	module_manager.reset(new ModuleManager());
	if (module_manager->initialize()) {
		LOG_INFO("[MAIN] PCA9685 modules initialized successfully\n");
		LOG_INFO("[MAIN] Loading saved modules configurations...\n");
		for (int i = 0; i < module_manager->getModuleCount(); i++) {
			const PCA9685Module* module = module_manager->getModule(i);
			if (module && module->isInitialized()) {
				LOG_INFO("[MAIN] Loading module %d configuration\n", i);
				storage_manager->load_module_config(i);

				// Load LED configs for this module
				LOG_INFO("[MAIN] Loading saved LEDs configurations for module %d...\n", i);
				for (int j = 0; j < module->getLedCount(); j++) {
					storage_manager->load_led_config(i, j);
				}
			} else {
				LOG_WARNING("[MAIN] Skipping configuration load for module %d (not initialized)\n", i);
			}
		}
		module_manager->printModuleInfo();
	} else {
		LOG_ERROR("[MAIN] PCA9685 modules initialization failed\n");
	}

	// Initialize program manager
	program_manager.reset(new ProgramManager());
	if (program_manager->initialize()) {
		LOG_INFO("[MAIN] Program manager initialized successfully\n");
	} else {
		LOG_ERROR("[MAIN] Program manager initialization failed\n");
	}

	// Setup WiFi connection with storage-based credentials
	network_manager.reset(new NetworkManager());

	// Try to load WiFi credentials from storage
	std::string stored_ssid, stored_password;
	bool has_stored_credentials = storage_manager->load_wifi_credentials(stored_ssid, stored_password);

	if (has_stored_credentials) {
		LOG_INFO("[MAIN] Found stored WiFi credentials for: %s\n", stored_ssid.c_str());
		
		NetworkManager::Credentials credentials;
		credentials.ssid = stored_ssid;
		credentials.password = stored_password;
		
		// Try to connect with stored credentials
		if (network_manager->initialize(credentials, 30)) {
			LOG_INFO("[MAIN] Network manager initialized successfully with stored credentials\n");
		} else {
			LOG_WARNING("[MAIN] Failed to connect with stored credentials\n");
			LOG_INFO("[MAIN] Please configure WiFi via web interface at /config\n");
		}
	} else {
		LOG_INFO("[MAIN] No stored WiFi credentials found\n");
	}

	// Start WiFi configuration portal
	if (!network_manager->startConfigurationPortal()) {
		LOG_ERROR("[MAIN] Warning: Could not start configuration portal\n");
	}

	// Setup captive DNS server for configuration portal
	captive_dns_server.reset(new CaptiveDNSServer());
	CaptiveDNSServer::Config dns_config;
	dns_config.port = 53;
	dns_config.redirect_domain = "*";
	dns_config.ttl = 60;

	if (captive_dns_server->initialize(dns_config)) {
		if (captive_dns_server->start()) {
			LOG_INFO("[MAIN] Captive DNS server started successfully\n");
		} else {
			LOG_WARNING("[MAIN] Could not start captive DNS server\n");
		}
	} else {
		LOG_ERROR("[MAIN] Failed to initialize captive DNS server\n");
	}

	// Setup OTA manager
	ota_manager.reset(new OTAManager());
	OTAManager::Config ota_config;
	ota_config.hostname = "emfao-led_controller";
	ota_config.password = ""; // Optionnel, laisser vide pour pas d'auth
	ota_config.auto_reboot = true;
	ota_config.timeout_ms = 60000; // 60 secondes timeout

	// Set up callbacks for monitoring
	ota_manager->setProgressCallback([](uint8_t progress, size_t total, size_t current) {
		LOG_INFO("[MAIN] OTA Progress: %u%% (%zu/%zu bytes)\n", progress, current, total);
	});

	ota_manager->setStatusCallback([](OTAManager::Status status, const std::string& message) {
		LOG_INFO("[MAIN] OTA Status Changed: %s - %s\n", 
		ota_manager->getStatusString().c_str(), message.c_str());
	});

	if (ota_manager->initialize(ota_config)) {
		if (ota_manager->start()) {
			LOG_INFO("[MAIN] OTA service started successfully\n");
		} else {
			LOG_ERROR("[MAIN] Failed to start OTA service\n");
		}
	} else {
		LOG_ERROR("[MAIN] Failed to initialize OTA manager\n");
	}

	// Setup web server 
	if (web_server.initialize()) {
		if (web_server.start()) {
			LOG_INFO("[MAIN] Web server started successfully\n");
		} else {
			LOG_ERROR("[MAIN] Failed to start web server\n");
		}
	} else {
		LOG_ERROR("[MAIN] Failed to initialize web server\n");
	}

	LOG_INFO("[MAIN] System initialization complete\n");
	LOG_INFO("[MAIN] Free heap: %d KB\n", ESP.getFreeHeap() / 1024);
}

void loop() {
    	unsigned long currentMillis = millis();

	// === Program Manager Update ===
	static unsigned long lastProgramUpdate = 0;
	if (currentMillis - lastProgramUpdate >= 10) { // Update à 100Hz pour fluidité
		program_manager->update(currentMillis);
		lastProgramUpdate = currentMillis;
	}
	
	// === Check OTA ===
	ota_manager->handle();

	// === Handle DNS requests for captive portal ===
	if (captive_dns_server && captive_dns_server->isActive()) {
		captive_dns_server->handleRequests();
	}

	// === System info print ===
	static unsigned long lastInfoPrint = 0;
	if (currentMillis - lastInfoPrint >= 30000) { // 30 sec
		print_system_info();
		lastInfoPrint = currentMillis;
	}

	// === WiFi and Portal check ===
	static unsigned long lastWiFiCheck = 0;
	if (currentMillis - lastWiFiCheck >= 30000) { // Check every 5 sec (plus frequent)
		network_manager->checkConnection();
		lastWiFiCheck = currentMillis;
	}
}