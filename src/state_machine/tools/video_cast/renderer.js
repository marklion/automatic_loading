class TCPMonitorApp {
    constructor() {
        this.currentUrl = ''
        this.currentPrompt = ''
        this.tcpConnected = false
        this.dataCount = 0
        this.lastUpdateTime = null
        this.frameLoadStartTime = null

        this.init()
    }

    init() {
        this.bindElements()
        this.bindEvents()
        this.setupElectronIPC()
        this.updateTime()

        // 每秒更新时间
        setInterval(() => this.updateTime(), 1000)

        // 更新内存使用
        setInterval(() => this.updateMemoryUsage(), 5000)

        console.log('TCP监控应用已启动')
    }

    bindElements() {
        // 主要元素
        this.contentFrame = document.getElementById('content-frame')
        this.promptText = document.getElementById('prompt-text')
        this.urlInput = document.getElementById('url-input')

        // 按钮
        this.reconnectBtn = document.getElementById('reconnect-btn')
        this.settingsBtn = document.getElementById('settings-btn')
        this.copyUrlBtn = document.getElementById('copy-url')
        this.refreshFrameBtn = document.getElementById('refresh-frame')
        this.copyPromptBtn = document.getElementById('copy-prompt')

        // 状态显示
        this.connectionStatus = document.getElementById('connection-status')
        this.serverInfo = document.getElementById('server-info')
        this.dataCountElem = document.getElementById('data-count')
        this.lastUpdateElem = document.getElementById('last-update')
        this.currentTimeElem = document.getElementById('current-time')
        this.memoryUsageElem = document.getElementById('memory-usage')

        // 设置模态框
        this.settingsModal = document.getElementById('settings-modal')
        this.tcpHostInput = document.getElementById('tcp-host')
        this.tcpPortInput = document.getElementById('tcp-port')
        this.closeSettingsBtn = document.getElementById('close-settings')
        this.cancelSettingsBtn = document.getElementById('cancel-settings')
        this.saveSettingsBtn = document.getElementById('save-settings')

        // 消息提示
        this.toast = document.getElementById('toast')

        // iframe控制
        this.backBtn = document.getElementById('back-btn')
        this.forwardBtn = document.getElementById('forward-btn')
        this.homeBtn = document.getElementById('home-btn')
        this.frameLoading = document.getElementById('frame-loading')
    }

    bindEvents() {
        // 重新连接按钮
        this.reconnectBtn.addEventListener('click', () => this.reconnectTCP())

        // 设置按钮
        this.settingsBtn.addEventListener('click', () => this.openSettings())
        this.closeSettingsBtn.addEventListener('click', () => this.closeSettings())
        this.cancelSettingsBtn.addEventListener('click', () => this.closeSettings())
        this.saveSettingsBtn.addEventListener('click', () => this.saveSettings())

        // URL相关
        this.copyUrlBtn.addEventListener('click', () => this.copyToClipboard(this.currentUrl, 'URL'))
        this.refreshFrameBtn.addEventListener('click', () => this.refreshFrame())

        // 提示信息相关
        this.copyPromptBtn.addEventListener('click', () => this.copyToClipboard(this.currentPrompt, '提示信息'))

        // iframe控制
        this.backBtn.addEventListener('click', () => this.goBack())
        this.forwardBtn.addEventListener('click', () => this.goForward())
        this.homeBtn.addEventListener('click', () => this.goHome())

        // iframe加载事件
        this.contentFrame.addEventListener('load', () => this.onFrameLoaded())
        this.contentFrame.addEventListener('error', () => this.onFrameError())

        // 设置模态框外部点击关闭
        this.settingsModal.addEventListener('click', (e) => {
            if (e.target === this.settingsModal) {
                this.closeSettings()
            }
        })
    }

    setupElectronIPC() {
        if (!window.electronAPI) {
            console.error('Electron API 不可用')
            return
        }

        // TCP数据接收
        window.electronAPI.onTCPData((data) => this.handleTCPData(data))

        // TCP状态更新
        window.electronAPI.onTCPStatus((status) => this.updateTCPStatus(status.connected))

        // TCP错误处理
        window.electronAPI.onTCPError((error) => this.showError(error))

        // TCP配置接收
        window.electronAPI.onTCPConfig((config) => this.updateTCPConfigDisplay(config))

        // 打开设置
        window.electronAPI.onOpenSettings(() => this.openSettings())
    }

    handleTCPData(data) {
        this.dataCount++
        this.lastUpdateTime = new Date()

        // 更新URL
        if (data.url && data.url !== this.currentUrl) {
            this.currentUrl = data.url
            this.updateFrame(data.url)
            this.urlInput.value = data.url
        }

        // 更新提示信息
        if (data.prompt && data.prompt !== this.currentPrompt) {
            this.currentPrompt = data.prompt
            this.updatePrompt(data.prompt)
        }

        // 更新UI
        this.updateStatus()
        this.showToast('收到新数据', 'success')
    }

    updateFrame(url) {
        if (!url) return
        url = "https://dev_al.d8sis.cn" + url + "/";
        try {
            // 验证URL格式
            new URL(url)

            this.frameLoadStartTime = Date.now()
            this.frameLoading.style.display = 'flex'
            this.contentFrame.src = url
        } catch (error) {
            console.error('无效的URL:', url, error)
            this.showError(`无效的URL: ${url}`)
        }
    }

    updatePrompt(prompt) {
        if (!prompt) {
            this.promptText.innerHTML = `
        <div class="placeholder-text">
          <i class="fas fa-comment-slash"></i>
          <p>等待TCP服务器发送提示信息...</p>
        </div>
      `
            return
        }

        // 处理换行和基本格式化
        const formattedPrompt = prompt
            .replace(/\n/g, '<br>')
            .replace(/\[(.*?)\]/g, '<strong>$1</strong>')

        this.promptText.innerHTML = formattedPrompt

        // 自动滚动到底部
        this.promptText.scrollTop = this.promptText.scrollHeight
    }

    updateTCPStatus(connected) {
        this.tcpConnected = connected

        const statusDot = this.connectionStatus.querySelector('.status-dot')
        const statusText = this.connectionStatus.querySelector('.status-text')

        if (connected) {
            statusDot.className = 'status-dot connected'
            statusText.textContent = '已连接'
            this.reconnectBtn.disabled = false
            this.reconnectBtn.innerHTML = '<i class="fas fa-plug"></i> 重新连接'
        } else {
            statusDot.className = 'status-dot disconnected'
            statusText.textContent = '已断开'
            this.reconnectBtn.disabled = false
            this.reconnectBtn.innerHTML = '<i class="fas fa-redo"></i> 重新连接'
        }
    }

    updateStatus() {
        // 更新服务器信息
        this.serverInfo.textContent = this.tcpConnected ?
            `已连接 | 数据包: ${this.dataCount}` :
            '未连接'

        // 更新数据计数
        this.dataCountElem.textContent = `数据包: ${this.dataCount}`

        // 更新最后更新时间
        if (this.lastUpdateTime) {
            const timeStr = this.lastUpdateTime.toLocaleTimeString('zh-CN')
            this.lastUpdateElem.textContent = `最后更新: ${timeStr}`
        }
    }

    updateTime() {
        const now = new Date()
        this.currentTimeElem.textContent = now.toLocaleTimeString('zh-CN')
    }

    updateMemoryUsage() {
        if (window.performance && window.performance.memory) {
            const usedMB = Math.round(window.performance.memory.usedJSHeapSize / 1024 / 1024)
            this.memoryUsageElem.textContent = `内存: ${usedMB} MB`
        }
    }

    updateTCPConfigDisplay(config) {
        this.tcpHostInput.value = config.HOST
        this.tcpPortInput.value = config.PORT
    }

    async reconnectTCP() {
        this.reconnectBtn.disabled = true
        this.reconnectBtn.innerHTML = '<i class="fas fa-sync fa-spin"></i> 连接中...'

        try {
            const success = await window.electronAPI.reconnectTCP()
            if (success) {
                this.showToast('正在重新连接...', 'info')
            }
        } catch (error) {
            this.showError('重新连接失败: ' + error.message)
            this.reconnectBtn.disabled = false
            this.reconnectBtn.innerHTML = '<i class="fas fa-plug"></i> 重新连接'
        }
    }

    openSettings() {
        this.settingsModal.style.display = 'block'
    }

    closeSettings() {
        this.settingsModal.style.display = 'none'
    }

    async saveSettings() {
        const host = this.tcpHostInput.value.trim()
        const port = parseInt(this.tcpPortInput.value)

        if (!host || !port || port < 1 || port > 65535) {
            this.showError('请输入有效的服务器地址和端口号')
            return
        }

        try {
            await window.electronAPI.updateTCPConfig({ host, port })
            this.closeSettings()
            this.showToast('设置已更新，正在重新连接...', 'success')
        } catch (error) {
            this.showError('保存设置失败: ' + error.message)
        }
    }

    onFrameLoaded() {
        const loadTime = Date.now() - this.frameLoadStartTime
        this.frameLoading.style.display = 'none'

        // 更新iframe控制按钮状态
        this.backBtn.disabled = !this.contentFrame.contentWindow?.history?.length
        this.forwardBtn.disabled = true // 简单实现，可扩展

        console.log(`iframe加载完成，耗时: ${loadTime}ms`)
    }

    onFrameError() {
        this.frameLoading.style.display = 'none'
        this.showError('页面加载失败，请检查URL')
    }

    goBack() {
        try {
            this.contentFrame.contentWindow.history.back()
        } catch (error) {
            console.error('无法后退:', error)
        }
    }

    goForward() {
        try {
            this.contentFrame.contentWindow.history.forward()
        } catch (error) {
            console.error('无法前进:', error)
        }
    }

    goHome() {
        if (this.currentUrl) {
            this.updateFrame(this.currentUrl)
        }
    }

    refreshFrame() {
        if (this.contentFrame.src) {
            this.contentFrame.contentWindow.location.reload()
        }
    }

    async copyToClipboard(text, label) {
        if (!text) {
            this.showError(`没有可复制的${label}`)
            return
        }

        try {
            await navigator.clipboard.writeText(text)
            this.showToast(`${label}已复制到剪贴板`, 'success')
        } catch (error) {
            console.error('复制失败:', error)
            this.showError(`复制${label}失败`)
        }
    }

    showToast(message, type = 'info') {
        const toast = this.toast
        toast.textContent = message
        toast.className = `show ${type}`

        setTimeout(() => {
            toast.className = toast.className.replace('show', '')
        }, 3000)
    }

    showError(message) {
        console.error('错误:', message)
        this.showToast(message, 'error')
    }
}

// 应用启动
document.addEventListener('DOMContentLoaded', () => {
    // 防止拖放文件
    document.addEventListener('dragover', (e) => e.preventDefault())
    document.addEventListener('drop', (e) => e.preventDefault())

    // 初始化应用
    new TCPMonitorApp()
})