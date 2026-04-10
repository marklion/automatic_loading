<template>
    <div v-if="should_show">
        <el-descriptions title="运行状态" :column="3" border>
            <template #extra>
                <el-button v-if="!isRecording" type="success" @click="startAudioStreaming">广播</el-button>
                <el-button v-else type="danger" @click="stopAudioProcessing">停止</el-button>
                <el-button type="warning" @click="enter_manual">手动</el-button>
                <el-button type="danger" @click="emergencyStop">急停</el-button>
                <el-button type="primary" @click="resetStateMachine">重置</el-button>
            </template>
            <el-descriptions-item label="当前状态">
                <el-tag type="primary">{{ sm_status.status }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="工作溜槽">
                <el-tag type="success">{{ sm_status.is_front_dropped ? '前' : '后' }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="车牌号">
                {{ sm_status.vehicle_info.plate }}
            </el-descriptions-item>
            <el-descriptions-item label="物料">
                {{ sm_status.vehicle_info.stuff_name }}
            </el-descriptions-item>
            <el-descriptions-item label="货厢边沿z坐标">
                {{ sm_status.side_z }}
            </el-descriptions-item>
            <el-descriptions-item label="应用配置套件">
                {{ sm_status.applied_kit }}
            </el-descriptions-item>
            <el-descriptions-item label="期望放料速度">
                {{ sm_status.expect_load_increase_speed }}
            </el-descriptions-item>
            <el-descriptions-item label="实际放料速度">
                {{ sm_status.current_load_increase_speed }}
            </el-descriptions-item>
        </el-descriptions>
        <el-row align="middle">
            <el-col :span="8">
                <el-progress status="success" :text-inside="true" :stroke-width="20"
                    :percentage="calcu_percentage(sm_status.vehicle_front_x, sm_status.basic_config.front_min_x, sm_status.basic_config.front_max_x)">
                    <template #default="{ percentage }">
                        <div class="label-text">{{ sm_status.basic_config.front_min_x }} - {{ sm_status.vehicle_front_x
                            }} - {{
                                sm_status.basic_config.front_max_x }}</div>
                    </template>
                </el-progress>
                <div class="label-text">车厢前壁位置</div>
            </el-col>
            <el-col :span="8">
                <el-progress status="warning" :text-inside="true" :stroke-width="20"
                    :percentage="calcu_percentage(sm_status.vehicle_tail_x, sm_status.basic_config.tail_min_x, sm_status.basic_config.tail_max_x)">
                    <template #default="{ percentage }">
                        <div class="label-text">{{ sm_status.basic_config.tail_min_x }} - {{ sm_status.vehicle_tail_x }}
                            - {{
                                sm_status.basic_config.tail_max_x }}</div>
                    </template>
                </el-progress>
                <div class="label-text">车厢后壁位置</div>
            </el-col>
            <el-col :span="8">
                <el-progress type="dashboard"
                    :percentage="calcu_percentage(sm_status.current_load, make_range_by_max(sm_status.basic_config.max_load).min, make_range_by_max(sm_status.basic_config.max_load).max)">
                    <template #default="{ percentage }">
                        <div>{{ sm_status.current_load }}<->{{ sm_status.basic_config.max_load }}</div>
                        <div>地磅示数</div>
                    </template>
                </el-progress>
                <el-progress type="dashboard"
                    :percentage="calcu_percentage(sm_status.stuff_full_offset, make_range_by_max(sm_status.basic_config.max_full_offset, -4).min, make_range_by_max(sm_status.basic_config.max_full_offset, -4).max)">
                    <template #default="{ percentage }">
                        <div>{{ sm_status.stuff_full_offset }}<->{{ sm_status.basic_config.max_full_offset }}</div>
                        <div>料堆满度</div>
                    </template>
                </el-progress>
            </el-col>
        </el-row>
    </div>
</template>

<script setup>
import { computed, getCurrentInstance, ref, onMounted } from "vue";
import { useStatusInfo } from "@/stores/status_info";
const instance = getCurrentInstance();
const isRecording = ref(false)
let ws = null;
let audioContext = null;
let audioWorkletNode = null;
let audioStream = null;

async function startAudioStreaming() {
    isRecording.value = true;
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
    isRecording.value = false;
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

onMounted(() => {
})
function make_range_by_max(max_value, min_value = 0) {
    let ret = {
        min: min_value,
        max: max_value / 0.8,
    }
    return ret;
}
function calcu_percentage(cur_value, min_value, max_value) {
    let range = max_value - min_value;
    let offset = cur_value - min_value;
    let percentage = (offset / range) * 100;
    if (percentage < 0) {
        percentage = 0;
    }
    if (percentage > 100) {
        percentage = 100;
    }
    return percentage;
}
async function emergencyStop() {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt e"
        );
    } catch (error) {
        console.log(error);
    }
}
async function resetStateMachine() {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt r"
        );
    } catch (error) {
        console.log(error);
    }
}
const statusInfoStore = useStatusInfo();
const sm_status = computed(() => {
    return statusInfoStore.sm || {};
});
const should_show = computed(() => {
    return Object.keys(sm_status.value).length > 0;
});

async function enter_manual() {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt m"
        );
    } catch (error) {
        console.log(error);
    }
}

</script>

<style lang="scss" scoped>
.label-text {
    text-align: center;
    font-size: 12px;
    color: red;
}
</style>
