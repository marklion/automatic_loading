
let ws = null;
let audioContext = null;
let audioWorkletNode = null;
let audioStream = null;
async function startAudioStreaming() {
    try {
        // 1. 获取麦克风权限
        audioStream = await navigator.mediaDevices.getUserMedia({
            audio: {
                sampleRate: 16000,
                channelCount: 1,
                echoCancellation: true,
                noiseSuppression: true
            }
        });

        // 2. 创建WebSocket连接
        ws = new WebSocket(`/audio-stream/`);
        ws.binaryType = 'arraybuffer'
        console.log(ws.readyState);
        ws.onopen = () => {
            console.log('WebSocket连接已建立');
            startAudioProcessing();
        };

        ws.onclose = () => {
            console.log('WebSocket连接已关闭');
            stopAudioProcessing();
        };

        ws.onerror = (error) => {
            console.error('WebSocket错误:', error);
        };

    } catch (error) {
        console.error('启动音频流失败:', error);
    }
}

async function startAudioProcessing() {
    try {
        // 3. 创建AudioContext
        audioContext = new AudioContext({
            sampleRate: 16000,
            latencyHint: 'interactive'
        });

        // 4. 等待AudioContext就绪
        if (audioContext.state === 'suspended') {
            await audioContext.resume();
        }

        // 5. 添加AudioWorklet处理器模块
        await audioContext.audioWorklet.addModule('/pcm-processor.js');

        // 6. 创建音频节点
        const source = audioContext.createMediaStreamSource(audioStream);
        audioWorkletNode = new AudioWorkletNode(audioContext, 'pcm-processor');

        // 7. 接收PCM数据并发送
        audioWorkletNode.port.onmessage = (event) => {
            if (ws && ws.readyState === WebSocket.OPEN) {
                // 发送Int16Array的buffer
                ws.send(event.data.buffer);
            }
        };

        // 8. 连接音频节点
        source.connect(audioWorkletNode);
        audioWorkletNode.connect(audioContext.destination); // 可选：监听自己的声音

        console.log('音频处理已启动');

    } catch (error) {
        console.error('音频处理启动失败:', error);
    }
}

function stopAudioProcessing() {
    if (audioWorkletNode) {
        audioWorkletNode.disconnect();
        audioWorkletNode = null;
    }

    if (audioContext && audioContext.state !== 'closed') {
        audioContext.close();
        audioContext = null;
    }

    if (audioStream) {
        audioStream.getTracks().forEach(track => track.stop());
        audioStream = null;
    }

    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.close();
    }
}
export { startAudioStreaming, stopAudioProcessing };