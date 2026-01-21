const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('electronAPI', {
    // TCP相关
    onTCPData: (callback) => ipcRenderer.on('tcp-data', (event, data) => callback(data)),
    onTCPStatus: (callback) => ipcRenderer.on('tcp-status', (event, status) => callback(status)),
    onTCPError: (callback) => ipcRenderer.on('tcp-error', (event, error) => callback(error)),
    onTCPConfig: (callback) => ipcRenderer.on('tcp-config', (event, config) => callback(config)),

    // 控制相关
    reconnectTCP: () => ipcRenderer.invoke('reconnect-tcp'),
    updateTCPConfig: (config) => ipcRenderer.invoke('update-tcp-config', config),

    // 设置相关
    onOpenSettings: (callback) => ipcRenderer.on('open-settings', (event) => callback()),

    // 窗口控制
    minimizeWindow: () => ipcRenderer.send('minimize-window'),
    maximizeWindow: () => ipcRenderer.send('maximize-window'),
    closeWindow: () => ipcRenderer.send('close-window')
})

// 安全地暴露版本信息
contextBridge.exposeInMainWorld('appInfo', {
    version: process.env.npm_package_version || '1.0.0',
    platform: process.platform
})