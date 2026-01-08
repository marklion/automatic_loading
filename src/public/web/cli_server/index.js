const express = require('express');
const path = require('path');
const fs = require('fs');
const { v4: uuidv4 } = require('uuid');
const { exec } = require('child_process');
const cors = require('cors');

const app = express();
const PORT = 35511;

app.use(cors());

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

app.get('/api/cli', async (req, res) => {
    let cli_cmd = decodeURIComponent(req.query.cmd || '');
    const tmpFile = path.join('/tmp', `cli_${uuidv4()}.txt`);
    fs.writeFileSync(tmpFile, cli_cmd + '\n');
    let full_cmd = `ad_cli ${tmpFile} | sed 's/^ad> //g'`;
    let output = '';
    try {
        output = await runCommand(full_cmd);
    } catch (error) {
        output = JSON.stringify({ error: error } );
    }
    fs.unlinkSync(tmpFile);
    res.send(output);
    console.log(`req cmd:${cli_cmd},resp:${output}`);
})

app.listen(PORT, () => {
    console.log(`CLI server is running on http://localhost:${PORT}`);
});