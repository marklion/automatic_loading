const express = require('express');
const path = require('path');
const fs = require('fs');
const { v4: uuidv4 } = require('uuid');
const { exec } = require('child_process');
const cors = require('cors');
const { DataSyncServer } = require('./websocket-data-sync.js');
const lockfile = require('proper-lockfile');

const app = express();
app.use(express.json());
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

app.get('/api/cli', async (req, res) => {
    let cli_cmd = decodeURIComponent(req.query.cmd || '');
    let output = await run_cli(cli_cmd);
    res.send(output);
})
app.post('/api/push_sm', async(req, res) => {
    ws_server.setData('sm_event', req.body);
    res.send({ status: 'ok' });
});

function print_spend(start_time, label='') {
    let end_time = Date.now();
    console.log(`${label} Spend time: ${end_time - start_time} ms`);
    return end_time;
}

async function update_status_info() {
    let start_time = Date.now();
    let status_info = {};
    let module_data_map = {
        'modbus_io': 'modbus_io list_devices json',
        'sm':'state_machine show_status json',
        'xlrd0':'xlrd read_offset 0',
        'xlrd1':'xlrd read_offset 1',
        'scale':'scale read_weight',
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
    setTimeout(update_status_info, 200);
}
setInterval(async () => {

}, 300);

app.listen(PORT, async () => {
    console.log(`CLI server is running on http://localhost:${PORT}`);
    await update_status_info();
});