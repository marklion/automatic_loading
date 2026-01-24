const express = require('express');
const path = require('path');
const fs = require('fs');
const { v4: uuidv4 } = require('uuid');
const { exec } = require('child_process');
const cors = require('cors');
const { DataSyncServer } = require('./websocket-data-sync.js');

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
console.log(gui_static_path);
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
    console.log(`req cmd:${cli_cmd},resp:${output}`);
})
app.post('/api/push_sm', async(req, res) => {
    ws_server.setData('sm_event', req.body);
    res.send({ status: 'ok' });
});

async function update_status_info() {
    let status_info = {};
    let module_data_map = {
        'modbus_io': 'modbus_io list_devices json',
        'sm':'state_machine show_status json',
        'xlrd0':'xlrd read_offset 0',
        'xlrd1':'xlrd read_offset 1',
    };
    for (let [module, cmd] of Object.entries(module_data_map)) {
        let output = await run_cli(cmd);
        console.log(`cmd:${cmd}, output:${output}`);
        try {
            status_info[module] = JSON.parse(output);
        } catch (error) {
            console.log(error);
        }
    }
    ws_server.setData('status_info', status_info);
    setTimeout(update_status_info, 200);
}

app.listen(PORT, async () => {
    console.log(`CLI server is running on http://localhost:${PORT}`);
    await update_status_info();
});