const express = require('express');
const path = require('path');
const fs = require('fs');
const { v4: uuidv4 } = require('uuid');
const { exec } = require('child_process');
const cors = require('cors');
const { DataSyncServer } = require('./websocket-data-sync.js');
const lockfile = require('proper-lockfile');
const net = require('net');
const multer = require('multer');

const app = express();
app.use(express.json());

// 配置 multer 用于临时存储上传的文件
const upload = multer({ dest: '/tmp/uploads/' });

const PORT = 35511;

app.use(cors());
const ws_server = new DataSyncServer({ port: 23312 });
async function runCommand(command) {
    return new Promise((resolve, reject) => {
        exec(command, (error, stdout, stderr) => {
            if (error) {
                reject(`Error: ${error.message}`);
                return;
            }
            if (stderr) {
                reject(`Stderr: ${stderr}`);
                return;
            }
            resolve(stdout);
        });
    });
}
let gui_static_path = path.join(__dirname, 'dist');
app.use(express.static(gui_static_path));

async function run_cli(cli_cmd) {
    const tmpFile = path.join('/tmp', `cli_${uuidv4()}.txt`);
    fs.writeFileSync(tmpFile, cli_cmd + '\n');
    let full_cmd = `ad_cli ${tmpFile} | sed 's/^ad> //g'`;
    let output = '';
    try {
        output = await runCommand(full_cmd);
    } catch (error) {
        output = JSON.stringify({ error: error });
    }
    fs.unlinkSync(tmpFile);
    return output;
}

// 文件上传接口
app.post('/api/upload_firmware', upload.single('file'), async (req, res) => {
    try {
        if (!req.file) {
            return res.status(400).json({ error: '没有文件被上传' });
        }

        // 检查文件名是否为 install.sh
        if (req.file.originalname !== 'install.sh') {
            // 删除临时文件
            fs.unlinkSync(req.file.path);
            return res.status(400).json({ error: '只允许上传 install.sh 文件' });
        }

        // 目标路径
        const targetPath = path.join('/root', 'install.sh');

        // 将文件从临时目录移动到 /root/
        fs.copyFileSync(req.file.path, targetPath);

        // 删除临时文件
        fs.unlinkSync(req.file.path);

        res.json({
            status: 'success',
            message: '文件上传成功',
            filepath: targetPath
        });
    } catch (error) {
        console.error('文件上传错误:', error);
        // 如果临时文件还存在，删除它
        if (req.file && fs.existsSync(req.file.path)) {
            fs.unlinkSync(req.file.path);
        }
        res.status(500).json({ error: '文件上传失败: ' + error.message });
    }
});

app.post('/api/update_system', async (req, res) => {
    res.send({ status: 'updating' });
    await runCommand('chmod +x /root/install.sh && kill -9 $(pgrep -f init_daemon)');
});

app.get('/api/cli', async (req, res) => {
    let cli_cmd = decodeURIComponent(req.query.cmd || '');
    let output = await run_cli(cli_cmd);
    res.send(output);
})
app.post('/api/push_sm', async (req, res) => {
    ws_server.setData('sm_event', req.body);
    res.send({ status: 'ok' });
});
const cast_info = {
    url: '',
    prompt:'',
    plate:'',
    weight:'',
    ann:{
        content:'',
        gap:-1,
    }
}
function send_cast_info() {
    ws_server.setData('video_cast', cast_info);
}
function initTcpClient() {
    const client = new net.Socket();
    let buffer = '';

    client.connect(47001, 'localhost', () => {
        console.log('TCP连接成功: localhost:47001');
    });

    client.on('data', (data) => {
        buffer += data.toString();

        // 尝试解析完整的JSON数据
        let lines = buffer.split('\n');
        buffer = lines[lines.length - 1]; // 保留未完成的部分

        for (let i = 0; i < lines.length - 1; i++) {
            const line = lines[i].trim();
            if (line) {
                try {
                    const jsonData = JSON.parse(line);
                    console.log('收到JSON数据:', jsonData);
                    cast_info.url = jsonData.url || '';
                    cast_info.prompt = jsonData.prompt || '';
                    cast_info.plate = jsonData.plate || '';
                    cast_info.weight = jsonData.weight || '';
                    cast_info.ann = jsonData.ann || { content: '', gap: -1 };
                    send_cast_info();
                } catch (error) {
                    console.log('JSON解析失败:', line, '错误:', error.message);
                }
            }
        }
    });

    client.on('error', (error) => {
        console.error('TCP连接错误:', error.message);
        // 5秒后重新连接
        setTimeout(initTcpClient, 5000);
    });

    client.on('close', () => {
        console.log('TCP连接已关闭，5秒后重新连接...');
        setTimeout(initTcpClient, 5000);
    });

    return client;
}

function print_spend(start_time, label = '') {
    let end_time = Date.now();
    console.log(`${label} Spend time: ${end_time - start_time} ms`);
    return end_time;
}

async function update_status_info() {
    let start_time = Date.now();
    let status_info = {};
    let module_data_map = {
        'modbus_io': 'modbus_io list_devices json',
        'sm': 'state_machine show_status json',
        'xlrd0': 'xlrd read_offset 0',
        'xlrd1': 'xlrd read_offset 1',
        'scale': 'scale read_weight',
        'drop_system': 'drop_system show_status',
    };
    for (let [module, cmd] of Object.entries(module_data_map)) {
        let output = await run_cli(cmd);
        start_time = print_spend(start_time, `Get ${module} status`);
        try {
            status_info[module] = JSON.parse(output);
        } catch (error) {
            console.log(error);
        }
    }
    ws_server.setData('status_info', status_info);
    if (status_info.sm.status != '空闲') {
        try {
            const lockPath = '/tmp/cloud.lock';
            // 获取文件锁
            const release = await lockfile.lock(lockPath, {
                retries: 5, // 重试次数
                retryWait: 10 // 每次重试等待时间（毫秒）
            });
            start_time = print_spend(start_time, 'wait lock');
            const data = await fs.promises.readFile('/tmp/cloud.bin');
            start_time = print_spend(start_time, 'read file');
            await release();
            ws_server.setData('pcd', { data: data.toString('base64') });
        } catch (error) {
            console.error('Error occurred:', error);
        }
    }
    send_cast_info();
    setTimeout(update_status_info, 200);
}

app.listen(PORT, async () => {
    console.log(`CLI server is running on http://localhost:${PORT}`);
    // 初始化TCP客户端
    initTcpClient();
    await update_status_info();
});