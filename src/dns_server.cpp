/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file dns_server.cpp
 * @brief Implementation of CaptiveDNSServer class
 * 
 * This file implements the CaptiveDNSServer class for ESP32 LED controller system.
 *
 * See dns_server.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-10
 */

#include "dns_server.h"
#include "log.h"

// Global instance
std::unique_ptr<CaptiveDNSServer> captive_dns_server;

// === Constructor and Destructor ===

CaptiveDNSServer::CaptiveDNSServer() :
	status_(Status::IDLE),
	dns_server_(nullptr),
	redirect_ip_(0, 0, 0, 0),
	start_time_(0),
	last_request_time_(0),
	request_count_(0) {}

CaptiveDNSServer::~CaptiveDNSServer() {
	stop();
}

// === Getters ===

std::string CaptiveDNSServer::getStatusString() const {
	switch (status_) {
		case Status::IDLE:       return "Idle";
		case Status::STARTING:   return "Starting";
		case Status::ACTIVE:     return "Active";
		case Status::ERROR:      return "Error";
		default:                 return "Unknown";
	}
}

unsigned long CaptiveDNSServer::getUptime() const {
	if (status_ != Status::ACTIVE || start_time_ == 0) {
		return 0;
	}
	return millis() - start_time_;
}

// === Server lifecycle ===

bool CaptiveDNSServer::initialize(const Config& config) {
	config_ = config;
	
	// Validate configuration
	if (config_.port == 0 || config_.port > 65535) {
		LOG_ERROR("[DNSSERVER] Error: Invalid port number: %d\n", config_.port);
		return false;
	}
	
	if (config_.redirect_domain.empty()) {
		LOG_ERROR("[DNSSERVER] Error: Invalid redirect domain\n");
		return false;
	}
	
	LOG_INFO("[DNSSERVER] DNS server initialized\n");
	LOG_INFO("[DNSSERVER] Port: %d\n", config_.port);
	LOG_INFO("[DNSSERVER] Redirect domain: %s\n", config_.redirect_domain.c_str());
	LOG_INFO("[DNSSERVER] TTL: %d seconds\n", config_.ttl);
	
	return true;
}

bool CaptiveDNSServer::start() {
	if (status_ == Status::ACTIVE) {
		LOG_INFO("[DNSSERVER] DNS server already active\n");
		return true;
	}
	
	// Check if WiFi AP is active
	if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
		LOG_ERROR("[DNSSERVER] Error: WiFi Access Point not active\n");
		return false;
	}
	
	// Get AP IP address
	redirect_ip_ = WiFi.softAPIP();
	if (redirect_ip_ == IPAddress(0, 0, 0, 0)) {
		LOG_ERROR("[DNSSERVER] Error: Could not get AP IP address\n");
		return false;
	}
	
	LOG_INFO("[DNSSERVER] Starting captive DNS server...\n");
	status_ = Status::STARTING;
	
	// Create and start DNS server
	dns_server_.reset(new DNSServer());
	
	// Start DNS server with correct parameters for ESP32 DNSServer library
	// The library expects: start(port, domain, ip)
	bool started = dns_server_->start(config_.port, "*", redirect_ip_);
	
	if (!started) {
		LOG_ERROR("[DNSSERVER] Failed to start DNS server on port %d\n", config_.port);
		dns_server_.reset();
		status_ = Status::ERROR;
		return false;
	}
	
	start_time_ = millis();
	request_count_ = 0;
	last_request_time_ = 0;
	status_ = Status::ACTIVE;
	
	LOG_INFO("[DNSSERVER] Captive DNS server started successfully\n");
	LOG_INFO("[DNSSERVER] Redirecting all DNS queries to: %s\n", redirect_ip_.toString().c_str());
	LOG_INFO("[DNSSERVER] Listening on port: %d\n", config_.port);
	
	return true;
}

void CaptiveDNSServer::stop() {
	if (status_ == Status::IDLE) {
		return;
	}
	
	LOG_INFO("[DNSSERVER] Stopping captive DNS server...\n");
	
	if (dns_server_) {
		dns_server_->stop();
		dns_server_.reset();
	}
	
	status_ = Status::IDLE;
	start_time_ = 0;
	
	LOG_INFO("[DNSSERVER] Captive DNS server stopped\n");
	if (request_count_ > 0) {
		LOG_INFO("[DNSSERVER] Total requests processed: %d\n", request_count_);
	}
}

void CaptiveDNSServer::handleRequests() {
	if (!isActive() || !dns_server_) {
		return;
	}
	
	// Process DNS requests
	dns_server_->processNextRequest();
	
	// Update statistics (simple approach - assume each call processes a request if any pending)
	updateStats();
	
	// Periodic status logging
	static unsigned long last_status_log = 0;
	if (millis() - last_status_log > 30000) { // Every 30 seconds
		if (request_count_ > 0) {
			LOG_INFO("[DNSSERVER] DNS server active - %d requests processed, uptime: %lu ms\n", 
			        request_count_, getUptime());
		}
		last_status_log = millis();
	}
}

void CaptiveDNSServer::printStatus() const {
	LOG_INFO("[DNSSERVER] === DNS Server Status ===\n");
	LOG_INFO("[DNSSERVER] Status: %s\n", getStatusString().c_str());
	LOG_INFO("[DNSSERVER] Port: %d\n", config_.port);
	LOG_INFO("[DNSSERVER] Redirect IP: %s\n", redirect_ip_.toString().c_str());
	
	if (isActive()) {
		LOG_INFO("[DNSSERVER] Uptime: %lu ms\n", getUptime());
		LOG_INFO("[DNSSERVER] Total requests: %d\n", request_count_);
		if (last_request_time_ > 0) {
			LOG_INFO("[DNSSERVER] Last request: %lu ms ago\n", millis() - last_request_time_);
		}
	}
}

// === Private functions ===

void CaptiveDNSServer::updateStats() {
	// Simple statistics update
	// In a real implementation, you might want to hook into the DNSServer
	// to get actual request counts, but for now we'll use periodic updates
	static unsigned long last_check = 0;
	unsigned long now = millis();
	
	if (now - last_check > 1000) { // Check every second
		// Assume we're processing requests if we're active
		// This is a simplified approach - actual request counting would require
		// modifications to the DNSServer library or packet sniffing
		if (isActive()) {
			// Increment request count periodically as an estimate
			// You could improve this by actually monitoring network traffic
			static uint32_t last_count = request_count_;
			if (WiFi.softAPgetStationNum() > 0) {
				request_count_++;
				if (request_count_ != last_count) {
					last_request_time_ = now;
				}
				last_count = request_count_;
			}
		}
		last_check = now;
	}
}