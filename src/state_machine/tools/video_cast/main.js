const { app, BrowserWindow, ipcMain, Menu, Tray } = require('electron')
const path = require('path')
const { initializeTCPClient } = require('./tcp-client')
const {exec} = require('child_process')

const TCP_CONFIG = {
    HOST: process.env.TCP_HOST || '192.168.0.4',  // 默认IP
    PORT: parseInt(process.env.TCP_PORT || '47001')  // 默认端口
}

let mainWindow = null
let tcpClient = null
app.commandLine.appendSwitch('disable-gpu')
app.commandLine.appendSwitch('disable-gpu-compositing')
app.commandLine.appendSwitch('disable-software-rasterizer')
app.commandLine.appendSwitch('disable-gpu-rasterization')
app.commandLine.appendSwitch('disable-zero-copy')
app.commandLine.appendSwitch('no-gpu')
app.commandLine.appendSwitch('no-sandbox')
app.commandLine.appendSwitch('enable-features', 'MediaSource,MediaSourceInWorkers')
app.commandLine.appendSwitch('disable-features', 'OutOfBlinkCors')
app.commandLine.appendSwitch('ignore-certificate-errors')
function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        minWidth: 800,
        minHeight: 600,
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            nodeIntegration: false,
            webSecurity: false,  // 注意：这会禁用web安全策略，仅用于开发
            allowRunningInsecureContent: true,  // 允许不安全内容
            webviewTag: true,
        },
        show: false, // 先隐藏，等加载完成再显示
        backgroundColor: '#1e1e1e'
    })

    // 加载主页面
    mainWindow.loadFile('index.html')

    mainWindow.webContents.openDevTools()

    // 窗口就绪后显示
    mainWindow.once('ready-to-show', () => {
        mainWindow.show()

        // 初始化TCP连接
        initializeTCPConnection()

        // 发送TCP配置到渲染进程
        mainWindow.webContents.send('tcp-config', TCP_CONFIG)
    })

    // 窗口关闭事件
    mainWindow.on('closed', () => {
        mainWindow = null
        if (tcpClient) {
            tcpClient.disconnect()
            tcpClient = null
        }
    })

    // 创建应用菜单
    createApplicationMenu()
}

// 初始化TCP连接
function initializeTCPConnection() {
    tcpClient = initializeTCPClient(TCP_CONFIG.HOST, TCP_CONFIG.PORT)

    // TCP事件处理
    tcpClient.on('data', (data) => {
        if (mainWindow) {
            mainWindow.webContents.send('tcp-data', data)
        }
        exec(`bash -c "/home/zczh/play.sh ${data.prompt}"`, {
            cwd: '/home/zczh/'
        }, function(error, stdout, stderr) {
            console.log(error, stdout, stderr);
        });
    })

    tcpClient.on('connected', () => {
        if (mainWindow) {
            mainWindow.webContents.send('tcp-status', { connected: true })
        }
    })

    tcpClient.on('disconnected', () => {
        if (mainWindow) {
            mainWindow.webContents.send('tcp-status', { connected: false })
        }
    })

    tcpClient.on('error', (error) => {
        console.error('TCP连接错误:', error)
        if (mainWindow) {
            mainWindow.webContents.send('tcp-error', error.message)
        }
    })
}

// 创建应用菜单
function createApplicationMenu() {
    const template = [
        {
            label: '文件',
            submenu: [
                {
                    label: '重新连接',
                    click: () => {
                        if (tcpClient) {
                            tcpClient.reconnect()
                        }
                    }
                },
                { type: 'separator' },
                {
                    label: '退出',
                    role: 'quit'
                }
            ]
        },
        {
            label: '查看',
            submenu: [
                { role: 'reload' },
                { role: 'forceReload' },
                { type: 'separator' },
                { role: 'toggleDevTools' }
            ]
        },
        {
            label: '设置',
            submenu: [
                {
                    label: 'TCP设置',
                    click: () => {
                        if (mainWindow) {
                            mainWindow.webContents.send('open-settings')
                        }
                    }
                }
            ]
        }
    ]

    const menu = Menu.buildFromTemplate(template)
    Menu.setApplicationMenu(menu)
}



// IPC通信处理
ipcMain.handle('reconnect-tcp', () => {
    if (tcpClient) {
        tcpClient.reconnect()
        return true
    }
    return false
})

ipcMain.handle('update-tcp-config', (event, config) => {
    TCP_CONFIG.HOST = config.host
    TCP_CONFIG.PORT = config.port

    if (tcpClient) {
        tcpClient.updateConfig(config.host, config.port)
        tcpClient.reconnect()
    }
    return true
})



// 应用生命周期
app.whenReady().then(createWindow)

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit()
    }
})

app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
        createWindow()
    }
})

// 防止多个实例
if (!app.requestSingleInstanceLock()) {
    app.quit()
} else {
    app.on('second-instance', () => {
        if (mainWindow) {
            if (mainWindow.isMinimized()) mainWindow.restore()
            mainWindow.focus()
        }
    })
}