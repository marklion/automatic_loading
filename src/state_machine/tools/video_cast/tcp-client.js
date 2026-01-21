const net = require('net')
const EventEmitter = require('events')

class TCPClient extends EventEmitter {
    constructor(host, port) {
        super()
        this.host = host
        this.port = port
        this.client = null
        this.reconnectInterval = 5000 // 5秒重连间隔
        this.maxReconnectAttempts = 10 // 最大重连次数
        this.reconnectAttempts = 0
        this.connected = false
        this.buffer = ''
        this.connection_establish_time = 0;
    }

    connect() {
        console.log(`正在连接到 ${this.host}:${this.port}...`)

        this.client = new net.Socket()

        this.client.setEncoding('utf8')

        this.client.on('connect', () => {
            console.log('TCP连接成功')
            this.connected = true
            this.reconnectAttempts = 0
            this.emit('connected')
            this.connection_establish_time = Date.now();
        })

        this.client.on('data', (data) => {
            this.buffer += data.toString()
            // 处理可能的分包情况
            let boundary
            while ((boundary = this.buffer.indexOf('\n')) !== -1) {
                const message = this.buffer.substring(0, boundary).trim()
                this.buffer = this.buffer.substring(boundary + 1)

                if (message) {
                    this.processMessage(message)
                }
            }
        })

        this.client.on('close', (hadError) => {
            let now = Date.now();
            let duration = (now - this.connection_establish_time) / 1000;
            console.log(`连接持续时间: ${duration} 秒`);
            console.log(`TCP连接关闭，有错误: ${hadError}`)
            this.connected = false
            this.emit('disconnected')

            // 自动重连
            this.scheduleReconnect()
        })

        this.client.on('error', (error) => {
            console.error('TCP连接错误:', error)
            this.emit('error', error)
        })

        this.client.on('timeout', () => {
            console.log('TCP连接超时')
            this.client.destroy()
        })

        try {
            this.client.connect(this.port, this.host)
        } catch (error) {
            console.error('连接失败:', error)
            this.scheduleReconnect()
        }
    }

    processMessage(message) {
        try {
            const data = JSON.parse(message)

            // 验证数据格式
            if (typeof data === 'object' && (data.url || data.prompt)) {
                this.emit('data', data)
            } else {
                console.warn('收到无效数据格式:', data)
            }
        } catch (error) {
            console.error('解析JSON失败:', error.message, '原始数据:', message)
        }
    }

    scheduleReconnect() {
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            this.reconnectAttempts++
            console.log(`尝试重连 (${this.reconnectAttempts}/${this.maxReconnectAttempts})...`)

            setTimeout(() => {
                if (!this.connected) {
                    this.connect()
                }
            }, this.reconnectInterval)
        } else {
            console.error('达到最大重连次数，停止重连')
        }
    }

    disconnect() {
        if (this.client) {
            this.client.destroy()
            this.client = null
        }
        this.connected = false
    }

    reconnect() {
        this.disconnect()
        this.reconnectAttempts = 0
        setTimeout(() => this.connect(), 1000)
    }

    updateConfig(host, port) {
        this.host = host
        this.port = port
    }

    send(data) {
        if (this.connected && this.client) {
            try {
                const message = typeof data === 'string' ? data : JSON.stringify(data)
                this.client.write(message + '\n')
                return true
            } catch (error) {
                console.error('发送数据失败:', error)
                return false
            }
        }
        return false
    }

    isConnected() {
        return this.connected
    }
}

// 导出单例
let tcpClientInstance = null

function initializeTCPClient(host, port) {
    if (!tcpClientInstance) {
        tcpClientInstance = new TCPClient(host, port)
        tcpClientInstance.connect()
    }
    return tcpClientInstance
}

function getTCPClient() {
    return tcpClientInstance
}

module.exports = {
    TCPClient,
    initializeTCPClient,
    getTCPClient
}