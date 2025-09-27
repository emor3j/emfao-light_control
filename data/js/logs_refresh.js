class LogViewer {
	constructor() {
		this.logsContainer = document.getElementById('logsContainer');
		this.refreshBtn = document.getElementById('refreshBtn');
		this.clearBtn = document.getElementById('clearBtn');
		this.autoScrollBtn = document.getElementById('autoScrollBtn');
		this.maxEntriesSelect = document.getElementById('maxEntries');
		this.autoRefreshSelect = document.getElementById('autoRefresh');
		this.statusInfo = document.getElementById('statusInfo');
		this.errorDisplay = document.getElementById('errorDisplay');
		this.errorMessage = document.getElementById('errorMessage');
		
		this.autoScrollEnabled = false;
		this.autoRefreshInterval = null;
		this.lastTimestamp = 0;
		
		this.initializeEventListeners();
		this.loadLogs();
	}
	
	initializeEventListeners() {
		this.refreshBtn.addEventListener('click', () => this.loadLogs());
		this.clearBtn.addEventListener('click', () => this.clearLogs());
		this.autoScrollBtn.addEventListener('click', () => this.toggleAutoScroll());
		
		this.maxEntriesSelect.addEventListener('change', () => this.loadLogs());
		this.autoRefreshSelect.addEventListener('change', () => this.setupAutoRefresh());
		
		// Setup initial auto-refresh
		this.setupAutoRefresh();
	}
	
	async loadLogs() {
		try {
			this.hideError();
			
			const maxEntries = parseInt(this.maxEntriesSelect.value);
			let url = '/api/logs';
			
			if (maxEntries > 0) {
				url += `?count=${maxEntries}`;
			}
			
			const response = await fetch(url);
			if (!response.ok) {
				throw new Error(`HTTP ${response.status}: ${response.statusText}`);
			}
			
			const data = await response.json();
			this.displayLogs(data.logs || []);
			this.updateStatus(data.stats || {});
			
			// Update last timestamp for incremental updates
			if (data.logs && data.logs.length > 0) {
				this.lastTimestamp = Math.max(...data.logs.map(log => log.timestamp));
			}
			
		} catch (error) {
			console.error('Error loading logs:', error);
			this.showError(`Failed to load logs: ${error.message}`);
			this.logsContainer.innerHTML = '<div class="loading">Failed to load logs</div>';
		}
	}
	
	displayLogs(logs) {
		if (!logs || logs.length === 0) {
			this.logsContainer.innerHTML = '<div class="loading">No logs available</div>';
			return;
		}
		
		// Remember scroll position for auto-scroll
		const wasAtBottom = this.isScrolledToBottom();
		
		this.logsContainer.innerHTML = '';
		
		logs.forEach(log => {
			const logEntry = document.createElement('div');
			
			const timestamp = this.formatTimestamp(log.timestamp);
			const message = this.escapeHtml(log.message || '');
			const levelName = this.getLevelName(log.level);

			logEntry.className = `log-entry log-level-${levelName}`;
			logEntry.innerHTML = `<span class="log-timestamp">[${timestamp}]</span><span class="log-level">[${levelName}]</span><span class="log-message">${message}</span>`;
			
			this.logsContainer.appendChild(logEntry);
		});
		
		// Auto-scroll to bottom if enabled and was at bottom
		if (this.autoScrollEnabled && wasAtBottom) {
			this.scrollToBottom();
		}
	}
	
	updateStatus(stats) {
		const bufferStatus = document.getElementById('bufferStatus');
		const lastUpdate = document.getElementById('lastUpdate');
		
		if (stats.total_entries !== undefined) {
			const utilization = stats.buffer_utilization || 0;
			bufferStatus.textContent = `${stats.total_entries} entries (${utilization}% full)`;
		} else {
			bufferStatus.textContent = 'Unknown';
		}
		
		lastUpdate.textContent = new Date().toLocaleTimeString();
	}
	
	async clearLogs() {
		if (!confirm('Are you sure you want to clear all logs?')) {
			return;
		}
		
		try {
			const response = await fetch('/api/logs', { method: 'DELETE' });
			if (!response.ok) {
				throw new Error(`HTTP ${response.status}: ${response.statusText}`);
			}
			
			this.logsContainer.innerHTML = '<div class="loading">Logs cleared</div>';
			this.lastTimestamp = 0;
			setTimeout(() => this.loadLogs(), 1000);
			
		} catch (error) {
			console.error('Error clearing logs:', error);
			this.showError(`Failed to clear logs: ${error.message}`);
		}
	}
	
	toggleAutoScroll() {
		this.autoScrollEnabled = !this.autoScrollEnabled;
		
		if (this.autoScrollEnabled) {
			this.autoScrollBtn.textContent = '📜 Auto-scroll ON';
			this.autoScrollBtn.classList.add('auto-scroll-enabled');
			this.scrollToBottom();
		} else {
			this.autoScrollBtn.textContent = '📜 Auto-scroll';
			this.autoScrollBtn.classList.remove('auto-scroll-enabled');
		}
	}
	
	setupAutoRefresh() {
		// Clear existing interval
		if (this.autoRefreshInterval) {
			clearInterval(this.autoRefreshInterval);
			this.autoRefreshInterval = null;
		}
		
		const seconds = parseInt(this.autoRefreshSelect.value);
		const statusElement = document.getElementById('autoRefreshStatus');
		
		if (seconds > 0) {
			this.autoRefreshInterval = setInterval(() => this.loadLogs(), seconds * 1000);
			statusElement.textContent = `Every ${seconds} seconds`;
		} else {
			statusElement.textContent = 'Disabled';
		}
	}
	
	formatTimestamp(timestamp) {
		// Convert milliseconds to human readable format
		const totalSeconds = Math.floor(timestamp / 1000);
		const hours = Math.floor(totalSeconds / 3600);
		const minutes = Math.floor((totalSeconds % 3600) / 60);
		const seconds = totalSeconds % 60;
		
		return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
	}
	
	escapeHtml(text) {
		const div = document.createElement('div');
		div.textContent = text;
		return div.innerHTML;
	}
	
	isScrolledToBottom() {
		const threshold = 50; // pixels from bottom
		return this.logsContainer.scrollTop + this.logsContainer.clientHeight >= 
			this.logsContainer.scrollHeight - threshold;
	}
	
	scrollToBottom() {
		this.logsContainer.scrollTop = this.logsContainer.scrollHeight;
	}
	
	showError(message) {
		this.errorMessage.textContent = message;
		this.errorDisplay.style.display = 'block';
	}
	
	hideError() {
		this.errorDisplay.style.display = 'none';
	}

	getLevelName(level) {
		switch (level) {
			case 0: return 'DEBUG';
			case 1: return 'INFO';
			case 2: return 'WARN';
			case 3: return 'ERROR';
			default: return 'INFO';
		}
	}
}

// Initialize log viewer when page loads
document.addEventListener('DOMContentLoaded', () => {
	new LogViewer();
});