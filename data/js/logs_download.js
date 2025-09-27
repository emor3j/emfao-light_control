// logs_download.js - Gestion du téléchargement des logs

class LogDownloader {
    constructor() {
        this.downloadBtn = document.getElementById('downloadBtn');
        this.initializeEventListeners();
    }
    
    initializeEventListeners() {
        if (this.downloadBtn) {
            this.downloadBtn.addEventListener('click', () => this.downloadLogs());
        }
    }
    
    async downloadLogs() {
        try {
            this.setDownloadingState(true);
            
            // Get ALL logs
            const response = await fetch('/api/logs');
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            const data = await response.json();
            const logs = data.logs || [];
            
            if (logs.length === 0) {
                alert('Aucun log disponible pour le téléchargement');
                return;
            }
            
            // Format logs for download
            const logContent = this.formatLogsForDownload(logs, data.stats);
            
            // Download the file
            this.downloadTextFile(logContent);
            
        } catch (error) {
            console.error('Erreur lors du téléchargement:', error);
            alert(`Échec du téléchargement: ${error.message}`);
        } finally {
            this.setDownloadingState(false);
        }
    }
    
    formatLogsForDownload(logs, stats) {
        const now = new Date();
        const timestamp = now.toISOString().replace(/[:.]/g, '-').slice(0, -5);
        
        let content = `ESP32 LED Controller - Logs Système\n`;
        content += `Généré le: ${now.toLocaleString('fr-FR')}\n`;
        content += `Nombre d'entrées: ${logs.length}\n`;
        
        if (stats) {
            content += `Utilisation du buffer: ${stats.buffer_utilization}%\n`;
            content += `Total entrées en mémoire: ${stats.total_entries}\n`;
        }
        
        content += `${'='.repeat(60)}\n\n`;
        
        logs.forEach(log => {
            const timestamp = this.formatTimestamp(log.timestamp);
            const level = this.getLevelName(log.level);
            content += `[${timestamp}] [${level.padEnd(5)}] ${log.message}`;
            
            if (!log.message.endsWith('\n')) {
                content += '\n';
            }
        });
        
        return content;
    }
    
    downloadTextFile(content) {
        // Create file name
        const now = new Date();
        const dateStr = now.toISOString().slice(0, 19).replace(/[:.]/g, '-');
        const filename = `esp32-logs-${dateStr}.txt`;
        
        // Download file
        const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
        const url = window.URL.createObjectURL(blob);
        
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.style.display = 'none';
        
        document.body.appendChild(a);
        a.click();
        
        // Cleanup
        document.body.removeChild(a);
        window.URL.revokeObjectURL(url);
    }
    
    formatTimestamp(timestamp) {
        // Convert millisecondes to HH:MM:SS format
        const totalSeconds = Math.floor(timestamp / 1000);
        const hours = Math.floor(totalSeconds / 3600);
        const minutes = Math.floor((totalSeconds % 3600) / 60);
        const seconds = totalSeconds % 60;
        
        return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
    }
    
    getLevelName(level) {
        switch (level) {
            case 0: return 'DEBUG';
            case 1: return 'INFO';
            case 2: return 'WARN';
            case 3: return 'ERROR';
            default: return 'UNKN';
        }
    }
    
    setDownloadingState(isDownloading) {
        if (this.downloadBtn) {
            if (isDownloading) {
                this.downloadBtn.textContent = '⏳ Téléchargement...';
                this.downloadBtn.disabled = true;
                this.downloadBtn.classList.add('btn-downloading');
            } else {
                this.downloadBtn.textContent = '💾 Download';
                this.downloadBtn.disabled = false;
                this.downloadBtn.classList.remove('btn-downloading');
            }
        }
    }
}

// Initialiser le téléchargeur quand la page est chargée
document.addEventListener('DOMContentLoaded', () => {
    new LogDownloader();
});