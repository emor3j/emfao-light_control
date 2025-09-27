class WiFiConfig {
    constructor() {
        this.init();
    }

    init() {
        this.loadWiFiStatus();
        this.setupEventListeners();
        this.scanNetworks();
    }

    setupEventListeners() {
        document.getElementById('scan-btn').addEventListener('click', () => {
            this.scanNetworks();
        });

        document.getElementById('ssid-select').addEventListener('change', (e) => {
            const manualGroup = document.getElementById('manual-ssid-group');
            if (e.target.value === 'manual') {
                manualGroup.style.display = 'block';
            } else {
                manualGroup.style.display = 'none';
            }
        });

        document.getElementById('wifi-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.saveWiFiConfig();
        });
    }

    async loadWiFiStatus() {
        try {
            const response = await fetch('/api/wifi/status');
            const data = await response.json();
            
            const statusDiv = document.getElementById('wifi-status');
            
            if (data.connected) {
                statusDiv.innerHTML = `
                    <h3>✅ Connected</h3>
                    <p><strong>Network:</strong> ${data.ssid}</p>
                    <p><strong>IP Address:</strong> ${data.ip_address}</p>
                    <p><strong>Signal:</strong> ${data.signal_strength} dBm</p>
                    <p><strong>MAC:</strong> ${data.mac_address}</p>
                `;
                statusDiv.className = 'status-box status-success';
            } else {
                statusDiv.innerHTML = `
                    <h3>❌ Not Connected</h3>
                    <p>Configure WiFi credentials to connect</p>
                    <p><strong>Credentials stored:</strong> ${data.credentials_stored ? 'Yes' : 'No'}</p>
                `;
                statusDiv.className = 'status-box status-error';
            }
        } catch (error) {
            console.error('Failed to load WiFi status:', error);
            document.getElementById('wifi-status').innerHTML = 
                '<p>❌ Failed to load WiFi status</p>';
        }
    }

    async scanNetworks() {
        const scanBtn = document.getElementById('scan-btn');
        const select = document.getElementById('ssid-select');
        
        scanBtn.textContent = '🔄 Scanning...';
        scanBtn.disabled = true;
        
        try {
            // Start the scan
            let response = await fetch('/api/wifi/scan');
            let data = await response.json();
            
            if (data.scanning) {
                // Poll for results
                await this.pollScanResults(select);
            } else if (data.networks) {
                // Immediate results (shouldn't happen with async)
                this.updateNetworkList(select, data);
            }
        } catch (error) {
            console.error('Failed to scan networks:', error);
            this.showMessage('Failed to scan networks', 'error');
        } finally {
            scanBtn.textContent = '🔄 Scan';
            scanBtn.disabled = false;
        }
    }

    async pollScanResults(select) {
        const maxAttempts = 20; // Max 20 seconds
        let attempts = 0;
        
        while (attempts < maxAttempts) {
            await new Promise(resolve => setTimeout(resolve, 1000)); // Wait 1 second
            attempts++;
            
            try {
                const response = await fetch('/api/wifi/scan');
                const data = await response.json();
                
                if (!data.scanning) {
                    // Scan completed
                    this.updateNetworkList(select, data);
                    if (data.count > 0) {
                        this.showMessage(`Found ${data.count} networks`, 'success');
                    } else {
                        this.showMessage('No networks found', 'warning');
                    }
                    return;
                }
                
                // Update progress message
                this.showMessage(`Scanning... (${attempts}s)`, 'info');
                
            } catch (error) {
                console.error('Error polling scan results:', error);
                break;
            }
        }
        
        // Timeout
        this.showMessage('Scan timeout', 'error');
    }

    updateNetworkList(select, data) {
        // Clear existing options
        select.innerHTML = `<option value="">Select a network...</option>`;
        
        if (data.networks && data.networks.length > 0) {
            // Add networks
            data.networks.forEach(network => {
                const option = document.createElement('option');
                option.value = network.ssid;
                option.textContent = `${network.ssid} (${network.rssi} dBm)`;
                select.appendChild(option);
            });
        }
        
        // Add manual option
        const manualOption = document.createElement('option');
        manualOption.value = 'manual';
        manualOption.textContent = '--- Enter manually ---';
        select.appendChild(manualOption);
    }

    async saveWiFiConfig() {
        const ssidSelect = document.getElementById('ssid-select');
        const manualSsid = document.getElementById('manual-ssid');
        const password = document.getElementById('password');
        const saveBtn = document.getElementById('save-btn');
        
        let ssid;
        if (ssidSelect.value === 'manual') {
            ssid = manualSsid.value.trim();
        } else {
            ssid = ssidSelect.value;
        }
        
        if (!ssid || !password.value) {
            this.showMessage('Please enter both SSID and password', 'error');
            return;
        }
        
        saveBtn.textContent = '💾 Saving...';
        saveBtn.disabled = true;
        
        try {
            const response = await fetch('/api/wifi/config', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    ssid: ssid,
                    password: password.value
                })
            });
            
            const data = await response.json();
            
            if (data.success) {
                if (data.connected) {
                    this.showMessage(`✅ Connected to ${ssid}! IP: ${data.ip_address}`, 'success');
                    setTimeout(() => {
                        this.loadWiFiStatus();
                    }, 2000);
                } else {
                    this.showMessage(`⚠️ Credentials saved but connection failed. Please check password.`, 'warning');
                }
            } else {
                this.showMessage(`❌ Failed: ${data.error}`, 'error');
            }
        } catch (error) {
            console.error('Failed to save WiFi config:', error);
            this.showMessage('❌ Failed to save configuration', 'error');
        } finally {
            saveBtn.textContent = '💾 Save & Connect';
            saveBtn.disabled = false;
        }
    }

    showMessage(text, type) {
        const messageDiv = document.getElementById('wifi-message');
        messageDiv.textContent = text;
        messageDiv.className = `message-box message-${type}`;
        messageDiv.style.display = 'block';
        
        setTimeout(() => {
            messageDiv.style.display = 'none';
        }, 5000);
    }
}

// Initialize when page loads
document.addEventListener('DOMContentLoaded', () => {
    new WiFiConfig();
});