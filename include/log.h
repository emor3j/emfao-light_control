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
 * @file    log.h
 * @brief   Declaration of the LogManager class for Serial log capture and web display.
 * 
 * This file implements the LogManager class that captures Serial output
 * and stores it in a circular buffer for web-based log viewing. The class
 * uses template functions to provide logging macros with double output
 * (Serial + web buffer).
 * 
 * The LogManager architecture includes:
 * - Template-based logging functions for type safety
 * - Circular buffer for efficient memory usage
 * - Thread-safe log storage and retrieval
 * - Timestamped log entries
 * - Web API integration for real-time log viewing
 * - Hardware Serial preservation for development
 * 
 * @author  Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * @date    2025-09-20
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include <string>

/**
 * @enum LogLevel
 * @brief Log level enumeration for categorizing messages
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

/**
 * @struct LogEntry
 * @brief Structure representing a single log entry
 */
struct LogEntry {
    unsigned long timestamp;    ///< Timestamp in milliseconds since boot
    LogLevel level;            ///< Log level
    std::string message;       ///< Complete log message
    
    LogEntry(unsigned long ts, LogLevel lvl, const std::string& msg) 
        : timestamp(ts), level(lvl), message(msg) {}
};

/**
 * @class LogManager
 * @brief Serial output capture and web log display manager
 * 
 * The LogManager class provides logging functions that output to both
 * Serial (for development) and a circular buffer (for web display).
 * It uses template functions for type-safe variadic arguments.
 * 
 * Key Features:
 * - Template-based logging functions
 * - Circular buffer storage for memory efficiency
 * - Thread-safe operations for concurrent access
 * - Timestamped log entries with levels
 * - Hardware Serial preservation for development
 * - JSON API for web interface integration
 * - Configurable buffer size and retention
 */
class LogManager {
private:
    static LogManager* instance_;           ///< Singleton instance
    
    std::vector<LogEntry> log_buffer_;     ///< Circular log buffer
    size_t buffer_size_;                   ///< Maximum buffer entries
    size_t current_index_;                 ///< Current write position in circular buffer
    bool buffer_full_;                     ///< Flag indicating if buffer has wrapped
    
    std::string temp_buffer_;              ///< Temporary buffer for formatting

public:
    /**
     * @brief Get singleton instance
     * @return Reference to the LogManager instance
     */
    static LogManager& getInstance();
    
    /**
     * @brief Initialize LogManager with configuration
     * @param buffer_size Maximum number of log entries to store (default: 200)
     * @return true if initialization successful
     */
    bool initialize(size_t buffer_size = 200);
    
    // === Template Logging Methods ===
    
    /**
     * @brief Log a message with printf-style formatting
     * @tparam Args Variadic template arguments
     * @param level Log level
     * @param format Printf-style format string
     * @param args Arguments for formatting
     */
    template<typename... Args>
    void log(LogLevel level, const char* format, Args... args) {
        // Format the message
        int size = snprintf(nullptr, 0, format, args...);
        if (size > 0) {
            temp_buffer_.resize(size + 1);
            snprintf(&temp_buffer_[0], size + 1, format, args...);
            temp_buffer_.resize(size); // Remove null terminator
            
            // Add to log buffer
            addLogEntry(level, temp_buffer_);
            
            // Output to Serial for development
            Serial.printf("[%s] %s", getLevelString(level), temp_buffer_.c_str());
        }
    }
    
    /**
     * @brief Log a simple string message
     * @param level Log level
     * @param message Message to log
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Log a simple C-string message
     * @param level Log level
     * @param message Message to log
     */
    void log(LogLevel level, const char* message);
    
    // === Log Retrieval Methods ===
    
    /**
     * @brief Get all stored log entries
     * @return Vector of log entries (copy for thread safety)
     */
    std::vector<LogEntry> getLogs() const;
    
    /**
     * @brief Get recent log entries
     * @param count Maximum number of recent entries to return
     * @return Vector of recent log entries
     */
    std::vector<LogEntry> getRecentLogs(size_t count) const;
    
    /**
     * @brief Get logs from a specific timestamp
     * @param since_timestamp Timestamp in milliseconds since boot
     * @return Vector of log entries newer than timestamp
     */
    std::vector<LogEntry> getLogsSince(unsigned long since_timestamp) const;
    
    /**
     * @brief Clear all stored logs
     */
    void clearLogs();
    
    /**
     * @brief Get current buffer statistics
     * @param total_entries Total number of entries stored
     * @param buffer_utilization Percentage of buffer used (0-100)
     */
    void getBufferStats(size_t& total_entries, uint8_t& buffer_utilization) const;

private:
    /**
     * @brief Default constructor (private for singleton)
     */
    LogManager();
    
    /**
     * @brief Add a log entry to the buffer
     * @param level Log level
     * @param message Message to add
     */
    void addLogEntry(LogLevel level, const std::string& message);
    
    /**
     * @brief Get log level string representation
     * @param level Log level to convert
     * @return String representation of log level
     */
    static const char* getLevelString(LogLevel level);
};

// === CONVENIENT LOGGING MACROS ===

/**
 * @brief Log debug message with printf-style formatting
 * @param format Printf-style format string
 * @param ... Arguments for formatting
 */
#define LOG_DEBUG(format, ...) LogManager::getInstance().log(LogLevel::DEBUG, format, ##__VA_ARGS__)

/**
 * @brief Log info message with printf-style formatting
 * @param format Printf-style format string
 * @param ... Arguments for formatting
 */
#define LOG_INFO(format, ...) LogManager::getInstance().log(LogLevel::INFO, format, ##__VA_ARGS__)

/**
 * @brief Log warning message with printf-style formatting
 * @param format Printf-style format string
 * @param ... Arguments for formatting
 */
#define LOG_WARNING(format, ...) LogManager::getInstance().log(LogLevel::WARNING, format, ##__VA_ARGS__)

/**
 * @brief Log error message with printf-style formatting
 * @param format Printf-style format string
 * @param ... Arguments for formatting
 */
#define LOG_ERROR(format, ...) LogManager::getInstance().log(LogLevel::ERROR, format, ##__VA_ARGS__)

/**
 * @brief General log message with printf-style formatting (INFO level)
 * @param format Printf-style format string
 * @param ... Arguments for formatting
 */
#define LOG_PRINTF(format, ...) LogManager::getInstance().log(LogLevel::INFO, format, ##__VA_ARGS__)

/**
 * @brief Simple string logging (INFO level)
 * @param message String message to log
 */
#define LOG_PRINTLN(message) LogManager::getInstance().log(LogLevel::INFO, std::string(message) + "\n")

/**
 * @brief Global LogManager instance for direct access
 */
extern LogManager& logger;