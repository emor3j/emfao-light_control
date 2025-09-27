const fileInput = document.getElementById('firmwareFile');
const fileName = document.getElementById('fileName');
const uploadBtn = document.getElementById('uploadBtn');
const progressSection = document.getElementById('progressSection');
const progressFill = document.getElementById('progressFill');
const progressText = document.getElementById('progressText');
const statusMessage = document.getElementById('statusMessage');

let selectedFile = null;

// File selection handling
fileInput.addEventListener('change', function(e) {
	const file = e.target.files[0];
	if (file) {
		if (file.name.toLowerCase().endsWith('.bin')) {
			selectedFile = file;
			fileName.textContent = `Selected: ${file.name} (${(file.size / 1024 / 1024).toFixed(2)} MB)`;
			fileName.style.display = 'block';
			uploadBtn.disabled = false;
			showStatus('File selected successfully', 'info');
		} else {
			showStatus('Please select a .bin firmware file', 'error');
			resetFileSelection();
		}
	} else {
		resetFileSelection();
	}
});

// Upload handling
uploadBtn.addEventListener('click', function() {
	if (!selectedFile) {
		showStatus('Please select a firmware file first', 'error');
		return;
	}
	
	uploadFirmware(selectedFile);
});

function resetFileSelection() {
	selectedFile = null;
	fileName.style.display = 'none';
	uploadBtn.disabled = true;
}

function showStatus(message, type) {
	statusMessage.textContent = message;
	statusMessage.className = `status-message status-${type}`;
	statusMessage.style.display = 'block';
}

function hideStatus() {
	statusMessage.style.display = 'none';
}

function updateProgress(percent) {
	progressFill.style.width = percent + '%';
	progressText.textContent = percent + '%';
}

function showProgress() {
	progressSection.style.display = 'block';
	updateProgress(0);
}

function hideProgress() {
	progressSection.style.display = 'none';
}

async function uploadFirmware(file) {
	try {
		showStatus('Starting firmware upload...', 'info');
		showProgress();
		uploadBtn.disabled = true;
		
		const formData = new FormData();
		formData.append('firmware', file, file.name);
		
		const xhr = new XMLHttpRequest();
		
		xhr.upload.onprogress = function(e) {
			if (e.lengthComputable) {
				const percent = Math.round((e.loaded / e.total) * 100);
				updateProgress(percent);
			}
		};
		
		xhr.onload = function() {
			if (xhr.status === 200) {
				const response = JSON.parse(xhr.responseText);
				if (response.success) {
					updateProgress(100);
					showStatus('✅ Firmware uploaded successfully! Device is restarting...', 'success');
					setTimeout(() => {
						showStatus('Device restarted. You may need to refresh the page.', 'info');
					}, 5000);
				} else {
					showStatus(`❌ Upload failed: ${response.error}`, 'error');
					uploadBtn.disabled = false;
				}
			} else {
				showStatus(`❌ Upload failed: HTTP ${xhr.status}`, 'error');
				uploadBtn.disabled = false;
			}
		};
		
		xhr.onerror = function() {
			showStatus('❌ Network error during upload', 'error');
			uploadBtn.disabled = false;
		};
		
		xhr.open('POST', '/api/ota/upload');
		xhr.send(formData);
		
	} catch (error) {
		showStatus(`❌ Upload error: ${error.message}`, 'error');
		uploadBtn.disabled = false;
		hideProgress();
	}
}

// Load system information
async function loadSystemInfo() {
	try {
		const response = await fetch('/api/system');
		const data = await response.json();
		
		document.getElementById('freeHeap').textContent = data.system.memory.free_heap_kb + ' KB';
		document.getElementById('flashSize').textContent = data.system.flash.size_mb + ' MB';
		document.getElementById('chipModel').textContent = data.system.chip.model;
		document.getElementById('wifiSignal').textContent = data.wifi.rssi_dbm + ' dBm';
		document.getElementById('uptime').textContent = data.system.uptime_formatted;
		
		// Check if system is healthy for update
		const healthResponse = await fetch('/api/health');
		const healthData = await healthResponse.json();
		
		if (healthData.status !== 'healthy') {
			showStatus(`⚠️ System status: ${healthData.status}. Please check system health before updating.`, 'warning');
		}
		
	} catch (error) {
		console.error('Failed to load system info:', error);
		showStatus('Failed to load system information', 'warning');
	}
}

// Load system info when page loads
document.addEventListener('DOMContentLoaded', loadSystemInfo);

// Refresh system info every 30 seconds
setInterval(loadSystemInfo, 30000);