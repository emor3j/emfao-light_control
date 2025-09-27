/**
 * SPDX-FileCopyrightText: 2025 Jérôme SONRIER
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file log.cpp
 * @brief Implementation of LogManager class 
 * 
 * This file implements the LogManager class for ESP32 LED controller system.
 * 
 * See log.h for API documentation.
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-20
 */

#include "log.h"

// === Static Members ===
LogManager* LogManager::instance_ = nullptr;
// Remove std::mutex for ESP32 compatibility - use simple flag instead
static volatile bool log_mutex_locked = false;

// Global logger reference
LogManager& logger = LogManager::getInstance();

// Simple ESP32-compatible mutex replacement
class SimpleMutex {
public:
    void lock() {
        while (log_mutex_locked) {
            delay(1); // Yield to other tasks
        }
        log_mutex_locked = true;
    }
    
    void unlock() {
        log_mutex_locked = false;
    }
};

class SimpleLockGuard {
private:
    SimpleMutex& mutex_;
public:
    SimpleLockGuard(SimpleMutex& m) : mutex_(m) { mutex_.lock(); }
    ~SimpleLockGuard() { mutex_.unlock(); }
};

static SimpleMutex simple_mutex;

// === Constructor and Singleton ===

LogManager::LogManager() 
    : buffer_size_(500)
    , current_index_(0)
    , buffer_full_(false) {
    log_buffer_.reserve(buffer_size_);
    temp_buffer_.reserve(512); // Reserve space for formatting
}

LogManager& LogManager::getInstance() {
    if (!instance_) {
        instance_ = new LogManager();
    }
    return *instance_;
}

bool LogManager::initialize(size_t buffer_size) {
    
    buffer_size_ = buffer_size;
    
    // Reserve buffer space for efficiency
    log_buffer_.clear();
    log_buffer_.reserve(buffer_size_);
    
    current_index_ = 0;
    buffer_full_ = false;
    temp_buffer_.clear();
    
    return true;
}

// === Non-template Logging Methods ===

void LogManager::log(LogLevel level, const std::string& message) {
    addLogEntry(level, message);
    
    // Output to Serial for development
    SimpleLockGuard lock(simple_mutex);
    Serial.printf("[%s] %s", getLevelString(level), message.c_str());
    Serial.flush(); // Forcer l'envoi complet

}

void LogManager::log(LogLevel level, const char* message) {
    addLogEntry(level, std::string(message));

    // Output to Serial for development
    SimpleLockGuard lock(simple_mutex);
    Serial.printf("[%s] %s", getLevelString(level), message);
    Serial.flush(); // Forcer l'envoi complet
}

// === Private Implementation ===

void LogManager::addLogEntry(LogLevel level, const std::string& message) {
    unsigned long timestamp = millis();
    LogEntry entry(timestamp, level, message);
    
    if (log_buffer_.size() < buffer_size_) {
        // Buffer not full yet - just add
        log_buffer_.push_back(entry);
    } else {
        // Buffer full - circular replacement
        log_buffer_[current_index_] = entry;
        current_index_ = (current_index_ + 1) % buffer_size_;
        buffer_full_ = true;
    }
}

// === Log Retrieval Implementation ===

std::vector<LogEntry> LogManager::getLogs() const {
    if (!buffer_full_) {
        // Buffer not full - return in order
        return log_buffer_;
    } else {
        // Buffer full - reconstruct chronological order
        std::vector<LogEntry> ordered_logs;
        ordered_logs.reserve(buffer_size_);
        
        // Start from oldest entry (current_index_) and wrap around
        for (size_t i = 0; i < buffer_size_; i++) {
            size_t index = (current_index_ + i) % buffer_size_;
            ordered_logs.push_back(log_buffer_[index]);
        }
        
        return ordered_logs;
    }
}

std::vector<LogEntry> LogManager::getRecentLogs(size_t count) const {
    std::vector<LogEntry> all_logs = getLogs();
    
    if (count >= all_logs.size()) {
        return all_logs;
    }
    
    // Return last 'count' entries
    std::vector<LogEntry> recent_logs;
    recent_logs.reserve(count);
    
    size_t start_index = all_logs.size() - count;
    for (size_t i = start_index; i < all_logs.size(); i++) {
        recent_logs.push_back(all_logs[i]);
    }
    
    return recent_logs;
}

std::vector<LogEntry> LogManager::getLogsSince(unsigned long since_timestamp) const {  
    std::vector<LogEntry> all_logs = getLogs();
    std::vector<LogEntry> filtered_logs;
    
    for (const auto& entry : all_logs) {
        if (entry.timestamp > since_timestamp) {
            filtered_logs.push_back(entry);
        }
    }
    
    return filtered_logs;
}

void LogManager::clearLogs() {   
    log_buffer_.clear();
    current_index_ = 0;
    buffer_full_ = false;
}

void LogManager::getBufferStats(size_t& total_entries, uint8_t& buffer_utilization) const {
    total_entries = buffer_full_ ? buffer_size_ : log_buffer_.size();
    buffer_utilization = (total_entries * 100) / buffer_size_;
}

const char* LogManager::getLevelString(LogLevel level) {
	switch (level) {
		case LogLevel::DEBUG:   return "DEBUG";
		case LogLevel::INFO:    return "INFO";
		case LogLevel::WARNING: return "WARN";
		case LogLevel::ERROR:   return "ERROR";
		default:                return "UNKNOWN";
	}
}