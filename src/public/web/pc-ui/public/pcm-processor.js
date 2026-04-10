// pcm-processor.js - AudioWorklet处理器
class PCMProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.sampleRate = 16000;
        this.bufferSize = 1024; // 每次处理的采样数
        this.buffer = new Int16Array(this.bufferSize);
        this.bufferIndex = 0;

        // 每100ms发送一次（16000Hz * 0.1s = 1600个采样）
        this.targetInterval = 1600;
        this.sampleCount = 0;
    }

    process(inputs) {
        const input = inputs[0];
        if (input.length > 0) {
            const channelData = input[0]; // 单声道

            for (let i = 0; i < channelData.length; i++) {
                // 将float32转换为int16
                const sample = Math.max(-1, Math.min(1, channelData[i])); // 钳制到[-1, 1]
                this.buffer[this.bufferIndex++] = sample * 0x7FFF; // 转换为16位有符号整数

                // 当缓冲区填满时发送
                if (this.bufferIndex === this.bufferSize) {
                    this.port.postMessage(this.buffer.slice());
                    this.bufferIndex = 0;
                }

                // 每0.1秒发送一次（即使缓冲区未满）
                this.sampleCount++;
                if (this.sampleCount >= this.targetInterval) {
                    if (this.bufferIndex > 0) {
                        this.port.postMessage(this.buffer.slice(0, this.bufferIndex));
                        this.bufferIndex = 0;
                    }
                    this.sampleCount = 0;
                }
            }
        }
        return true; // 保持处理器运行
    }
}

registerProcessor('pcm-processor', PCMProcessor);