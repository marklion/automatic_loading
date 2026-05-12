<template>
    <div class="loading-system" :class="{ 'waiting-flash': systemState === 'waiting' || systemState === 'error' }">
        <!-- 顶部状态栏 -->
        <div class="status-bar">
            <div class="status-left">
            </div>
            <div class="system-info">
                <div class="info-strip">
                    <span class="info-value status info-status" :class="systemState">{{ systemStatus }}</span>
                    <span class="info-value info-material">{{ material }}</span>
                    <span class="info-value info-plate">{{ licensePlate }}</span>
                    <div class="drop-mode-switch control-switch-item">
                        <span class="drop-mode-label">放料模式</span>
                        <label class="switch">
                            <input type="checkbox" :checked="isAutoDropMode" @change="turn_on_off" />
                            <span class="slider"></span>
                        </label>
                        <span class="drop-mode-text">{{ isAutoDropMode ? '自动放料' : '手动放料' }}</span>
                    </div>
                </div>
            </div>
            <div class="status-clock">{{ currentTime }}</div>
        </div>

        <!-- 主界面 -->
        <div class="main-container">
            <div class="center-column">
                <!-- 中央控制区域 -->
                <div class="control-panel">
                    <div class="control-main-layout">
                        <div class="control-info-column">
                            <!-- 状态信息 -->


                            <!-- 进度条 -->
                            <div class="progress-container">
                                <div class="progress-header">
                                    <span>装载进度</span>
                                    <span>{{ loadedWeight }}/{{ totalWeight }}吨</span>
                                </div>
                                <div class="progress-bar">
                                    <div class="progress-fill" :style="{ width: `${progressPercentage}%` }"></div>
                                </div>
                                <div class="progress-percentage">{{ progressPercentage }}%</div>

                                <div class="fullness-progress">
                                    <div class="progress-header">
                                        <span>料堆满度</span>
                                        <span>{{ full_offset_progress }}%</span>
                                    </div>
                                    <div class="progress-bar full-progress-bar">
                                        <div class="progress-fill full-progress-fill"
                                            :style="{ width: `${full_offset_progress}%` }"></div>
                                    </div>
                                    <div class="progress-percentage full-progress-percentage">{{ full_offset_progress }}%</div>
                                </div>
                            </div>
                        </div>

                        <!-- 控制按钮 -->
                        <div class="control-buttons">
                            <button class="control-button emergency-stop" @click="handleEmergencyStop">
                                <div class="button-icon">!</div>
                                <div class="button-label">急停</div>
                            </button>

                            <button v-if="systemState != 'waiting'" class="control-button supplement" @click="changeManual">
                                <div class="button-icon">+</div>
                                <div class="button-label">手动</div>
                            </button>

                            <button v-else class="control-button supplement" @click="handleSupplement">
                                <div class="button-icon">+</div>
                                <div class="button-label">补料</div>
                            </button>

                            <button class="control-button reset" @click="handleReset">
                                <div class="button-icon">↺</div>
                                <div class="button-label">重置</div>
                            </button>

                            <button v-if="!is_recording" class="control-button broadcast" @click="my_startAudioStreaming">
                                <div class="button-icon">📢</div>
                                <div class="button-label">广播</div>
                            </button>
                            <button v-else class="control-button stop_broadcast" @click="my_stopAudioProcessing">
                                <div class="button-icon">⏹</div>
                                <div class="button-label">停止</div>
                            </button>
                        </div>
                    </div>

                </div>

                <div class="monitor-panel left-panel">
                    <LiveCamera></LiveCamera>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup>
import {
    ref,
    reactive,
    computed,
    onMounted,
    onUnmounted,
    getCurrentInstance
} from 'vue'
import {
    useStatusInfo
} from "@/stores/status_info";
import LiveCamera from "../../../../../live_camera/web/live_camera.vue";
import { startAudioStreaming, stopAudioProcessing } from "@/broadcast";

const instance = getCurrentInstance();
const statusInfoStore = useStatusInfo();
const sm_status = computed(() => {
    return statusInfoStore.sm || {};
});
const is_recording = ref(false)
// 状态管理
const systemState = computed(() => {
    let ret = 'stopped';
    switch (sm_status.value.status) {
        case "首堆":
        case "工作中":
        case "收尾":
            ret = 'loading';
            break;
        case "急停":
            ret = 'error';
            break;
        case "手动":
            ret = 'waiting';
            break;
    }
    return ret;
}); // loading, waiting, stopped, error
const systemStatus = computed(() => {
    return sm_status.value.status || '未知状态';
})

const statusText = computed(() => {
    return [systemStatus.value, material.value, licensePlate.value].join(' · ')
})

// 数据定义
const material = computed(() => {
    return sm_status.value.vehicle_info?.stuff_name || '未知物料'
})
const licensePlate = computed(() => {
    return sm_status.value.vehicle_info?.plate || '未知车牌'
})
const loadedWeight = computed(() => {
    return sm_status.value.current_load || 0
})
const totalWeight = computed(() => {
    return sm_status.value.basic_config ? sm_status.value.basic_config.max_load : 100
})
const currentTime = ref('')
const full_offset_progress = computed(() => {
    if (!sm_status.value.basic_config) {
        return 0;
    }
    const offset = sm_status.value.stuff_full_offset || 0;
    const max_offset = sm_status.value.basic_config.max_full_offset || 0;
    const min_offset = sm_status.value.basic_config.max_full_offset ? (sm_status.value.basic_config.max_full_offset - 1.2) : 0;
    if (offset >= max_offset) {
        return 100;
    } else if (offset <= min_offset) {
        return 0;
    } else {
        return Math.round(((offset - min_offset) / (max_offset - min_offset)) * 100);
    }
})

// 放料模式计算属性（自动放料: true，手动放料: false）
const isAutoDropMode = computed(() => {
    return statusInfoStore.drop_system?.system_on || false;
})

// 计算属性
const progressPercentage = computed(() => {
    return Math.round((loadedWeight.value / totalWeight.value) * 100)
})

async function turn_on_off() {
    await instance.appContext.config.globalProperties.$call_remote_cli(
        `drop_system turn_on_off "${isAutoDropMode.value ? "off" : "on"}"`
    );
}

// 定时器更新当前时间
let timer
const updateTime = () => {
    const now = new Date()
    currentTime.value = now.toLocaleTimeString('zh-CN', {
        hour12: false,
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    })
    if (currentTime.value.split(":")[1] == "00" && currentTime.value.split(":")[2] == "00") {
        window.location.reload();
    }
}
// 按钮处理函数
const handleEmergencyStop = async () => {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt e"
        );
    } catch (error) {
        console.log(error);
    }
}

const handleSupplement = async () => {
    try {
        let kit_config = await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine list_kits_json"
        );
        let cur_kit = kit_config.find(item => item.kit_name === sm_status.value.applied_kit);
        await instance.appContext.config.globalProperties.$call_remote_cli(
            `drop_system one_time_drop "${cur_kit['config_items']['CONFIG_ITEM_CONFIG_KIT_DS_INPUT_DEV']}"`
        );
    } catch (error) {
        console.log(error);
    }

}
const changeManual = async () => {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt m"
        );

    } catch (error) {
        console.log(error);
    }

}

const handleReset = async () => {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt r"
        );
    } catch (error) {
        console.log(error);
    }
}
async function my_startAudioStreaming() {
    is_recording.value = true;
    await startAudioStreaming();
}
async function my_stopAudioProcessing() {
    is_recording.value = false;
    await stopAudioProcessing();
}


// 生命周期
onMounted(() => {
    updateTime()
    timer = setInterval(updateTime, 1000)
})

onUnmounted(() => {
    if (timer) {
        clearInterval(timer)
    }
})
</script>

<style scoped>
/* 整体布局 */
.loading-system {
    width: 99vw;
    height: 90vh;
    background: linear-gradient(135deg, #0f0f0f 0%, #1a1a1a 100%);
    color: #e0e0e0;
    font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;
    display: flex;
    flex-direction: column;
    overflow: hidden;
}

.loading-system.waiting-flash {
    animation: waitingBackgroundFlash 1.2s ease-in-out infinite;
}

/* 顶部状态栏 */
.status-bar {
    background-color: rgba(0, 0, 0, 0.7);
    padding: 12px 20px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    border-bottom: 2px solid #333;
}

.status-left {
    display: flex;
    align-items: center;
    min-width: 0;
}

.status-indicator {
    width: 12px;
    height: 12px;
    border-radius: 50%;
    margin-right: 12px;
    box-shadow: 0 0 8px currentColor;
}

.status-indicator.loading {
    background-color: #4CAF50;
    animation: pulse 1.5s infinite;
}

.status-indicator.waiting {
    background-color: #FFC107;
}

.status-indicator.stopped {
    background-color: #36a2e1;
}

.status-indicator.error {
    background-color: #f31313;
    animation: blink 1s infinite;
}

.status-text {
    font-size: 18px;
    font-weight: 600;
    color: #4CAF50;
    text-shadow: 0 0 5px rgba(76, 175, 80, 0.5);
    letter-spacing: 1px;
    white-space: nowrap;
}

.status-clock {
    margin-left: 20px;
    font-size: 20px;
    font-weight: 600;
    color: #4CAF50;
    letter-spacing: 2px;
    white-space: nowrap;
}

/* 主容器 */
.main-container {
    flex: 1;
    display: flex;
    padding: 20px;
    gap: 20px;
    overflow: hidden;
}

.center-column {
    flex: 1;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 12px;
    overflow: hidden;
}

/* 监控面板 */
.monitor-panel {
    background: rgba(30, 30, 30, 0.8);
    border-radius: 8px;
    border: 1px solid #444;
    padding: 15px;
    display: flex;
    flex-direction: column;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
}

.left-panel {
    flex: 1;
    min-height: 300px;
}

.right-panel {
    width: 300px;
}

.panel-title {
    font-size: 16px;
    font-weight: 600;
    color: #4CAF50;
    margin-bottom: 15px;
    padding-bottom: 8px;
    border-bottom: 1px solid #444;
}

.data-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
    margin-bottom: 20px;
}

.drop-mode-switch {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 10px 0;
}

.drop-mode-label {
    font-size: 13px;
    color: #aaa;
}

.drop-mode-text {
    font-size: 13px;
    color: #fff;
    min-width: 60px;
}

.switch {
    position: relative;
    display: inline-block;
    width: 48px;
    height: 26px;
}

.switch input {
    opacity: 0;
    width: 0;
    height: 0;
}

.slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #555;
    border-radius: 26px;
    transition: 0.3s;
}

.slider:before {
    position: absolute;
    content: "";
    height: 20px;
    width: 20px;
    left: 3px;
    bottom: 3px;
    background-color: #fff;
    border-radius: 50%;
    transition: 0.3s;
}

.switch input:checked+.slider {
    background-color: #4caf50;
}

.switch input:checked+.slider:before {
    transform: translateX(22px);
}

.data-item {
    background: rgba(40, 40, 40, 0.7);
    border-radius: 6px;
    padding: 10px;
    border: 1px solid #555;
}

.data-label {
    font-size: 12px;
    color: #aaa;
    margin-bottom: 5px;
}

.data-value {
    font-size: 18px;
    font-weight: 600;
    color: #fff;
}

.data-value span {
    font-size: 12px;
    color: #aaa;
    margin-left: 2px;
}

.chart-container {
    flex: 1;
    background: rgba(20, 20, 20, 0.6);
    border-radius: 6px;
    padding: 10px;
    border: 1px solid #555;
}

.chart-title {
    font-size: 14px;
    color: #aaa;
    margin-bottom: 10px;
}

.chart-placeholder {
    width: 100%;
    height: calc(100% - 20px);
    background: linear-gradient(90deg, transparent 49%, #333 49%, transparent 51%),
        linear-gradient(0deg, transparent 49%, #333 49%, transparent 51%);
    background-size: 20px 20px;
    border-radius: 4px;
}

/* 中央控制面板 */
.control-panel {
    flex: 0 0 auto;
    display: flex;
    flex-direction: column;
    align-items: stretch;
    background: rgba(25, 25, 25, 0.7);
    border-radius: 8px;
    padding: 14px 16px;
    border: 1px solid #444;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
}

.control-main-layout {
    width: 100%;
    display: flex;
    flex-direction: row;
    gap: 12px;
    align-items: flex-start;
    justify-content: space-between;
}

.control-info-column {
    flex: 1 1 auto;
    width: auto;
    min-width: 0;
}

/* 系统信息 */
.system-info {
    width: 100%;
    background: rgba(40, 40, 40, 0.8);
    border-radius: 8px;
    padding: 12px;
    margin-bottom: 12px;
    border: 1px solid #555;
}

.info-strip {
    display: flex;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
}

.info-value {
    font-weight: 600;
    line-height: 1.2;
}

.info-status {
    font-size: 18px;
    padding: 2px 10px;
    border-radius: 4px;
    background-color: rgba(76, 175, 80, 0.2);
    color: #4CAF50;
}

.info-material {
    font-size: 15px;
    color: #ff4d4f;
}

.info-plate {
    font-size: 22px;
    color: #111;
    background: #f5d400;
    border-radius: 4px;
    padding: 4px 12px;
    letter-spacing: 1px;
}

/* 进度条 */
.progress-container {
    width: 100%;
    max-width: 860px;
    height: 96px;
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    margin-bottom: 0;
}

.progress-header {
    display: flex;
    justify-content: space-between;
    margin-bottom: 2px;
    font-size: 13px;
}

.progress-bar {
    height: 8px;
    background-color: #333;
    border-radius: 999px;
    overflow: hidden;
    border: 1px solid #555;
    margin-bottom: 2px;
}

.progress-fill {
    height: 100%;
    background: linear-gradient(90deg, #4CAF50, #8BC34A);
    border-radius: 999px;
    transition: width 0.5s ease;
}

.progress-percentage {
    text-align: right;
    font-size: 12px;
    font-weight: 600;
    line-height: 1;
    color: #4CAF50;
}

.fullness-progress {
    margin-top: 0;
}

.full-progress-bar {
    border-color: #805d00;
}

.full-progress-fill {
    background: linear-gradient(90deg, #ff9800, #ffd54f);
}

.full-progress-percentage {
    color: #ffb300;
}

/* 控制按钮 */
.control-buttons {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: 10px;
    width: 414px;
    flex: 0 0 414px;
    align-content: start;
}

.control-button {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    width: 96px;
    aspect-ratio: 1 / 1;
    border: none;
    border-radius: 50%;
    font-size: 16px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.3s ease;
    box-shadow: 0 10px 18px rgba(0, 0, 0, 0.35), 0 2px 6px rgba(0, 0, 0, 0.3);
    position: relative;
    overflow: hidden;
    justify-self: center;
    flex: 0 0 auto;
}

.control-button:hover {
    transform: translateY(-5px);
    box-shadow: 0 14px 22px rgba(0, 0, 0, 0.42), 0 4px 10px rgba(0, 0, 0, 0.35);
}

.control-button:active {
    transform: translateY(0);
}

.control-button:disabled {
    opacity: 0.7;
    cursor: not-allowed;
    transform: none;
}

.emergency-stop {
    background: linear-gradient(135deg, #d32f2f, #f44336);
    color: white;
}

.emergency-stop:disabled {
    background: linear-gradient(135deg, #9e9e9e, #757575);
}

.supplement {
    background: linear-gradient(135deg, #FFA000, #FFC107);
    color: #333;
}

.reset {
    background: linear-gradient(135deg, #1976D2, #2196F3);
    color: white;
}

.broadcast {
    background: linear-gradient(135deg, #388E3C, #4CAF50);
    color: white;
}

.stop_broadcast {
    background: linear-gradient(135deg, #d30c62, #bb0a95);
    color: white;
}

.button-icon {
    font-size: 28px;
    font-weight: 700;
    margin-bottom: 6px;
}

.button-label {
    font-size: 15px;
}

.control-switch-item {
    flex: 0 0 auto;
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 0 10px;
    height: 32px;
    border-radius: 48px;
    border: 1px solid #555;
    background: rgba(40, 40, 40, 0.7);
    white-space: nowrap;
}

.info-strip .control-switch-item {
    margin-left: auto;
}

.control-switch-item .drop-mode-label {
    font-size: 13px;
    color: #aaa;
}

.control-switch-item .drop-mode-text {
    font-size: 13px;
    color: #fff;
    min-width: 60px;
}

.button-status {
    position: absolute;
    bottom: 10px;
    font-size: 12px;
    background: rgba(0, 0, 0, 0.5);
    padding: 2px 8px;
    border-radius: 10px;
}

/* 右侧卡车图 */
.right-panel .truck-diagram {
    height: 200px;
    background: rgba(20, 20, 20, 0.6);
    border-radius: 6px;
    margin-bottom: 20px;
    position: relative;
    border: 1px solid #555;
    overflow: hidden;
}

.truck-outline {
    width: 100%;
    height: 100%;
    background: linear-gradient(90deg, transparent 10%, #333 10%, transparent 20%),
        linear-gradient(90deg, transparent 30%, #333 30%, #444 70%, transparent 70%),
        linear-gradient(90deg, transparent 80%, #333 80%, transparent 90%);
    background-size: 100% 20px;
    position: relative;
}

.loading-level {
    position: absolute;
    bottom: 0;
    left: 30%;
    width: 40%;
    background: linear-gradient(to top, rgba(76, 175, 80, 0.3), rgba(76, 175, 80, 0.7));
    transition: height 0.5s ease;
    border-top: 2px solid #4CAF50;
}

/* 底部信息栏 */
.footer {
    background-color: rgba(0, 0, 0, 0.7);
    padding: 10px 20px;
    display: flex;
    justify-content: space-between;
    border-top: 2px solid #333;
    font-size: 14px;
    color: #aaa;
}

/* 动画 */
@keyframes pulse {
    0% {
        opacity: 1;
    }

    50% {
        opacity: 0.6;
    }

    100% {
        opacity: 1;
    }
}

@keyframes blink {

    0%,
    100% {
        opacity: 1;
    }

    50% {
        opacity: 0.3;
    }
}

@keyframes waitingBackgroundFlash {

    0%,
    100% {
        background: linear-gradient(135deg, #ed0202 0%, #eb0505 100%);
    }

    50% {
        background: linear-gradient(135deg, #2a2200 0%, #3a3000 100%);
    }
}

@media (max-width: 768px) {
    .loading-system {
        width: 100vw;
        height: auto;
        min-height: 100vh;
    }

    .status-bar {
        padding: 10px 12px;
    }

    .status-text {
        font-size: 14px;
        letter-spacing: 0;
    }

    .status-clock {
        font-size: 16px;
        letter-spacing: 1px;
        margin-left: 10px;
    }

    .main-container {
        flex-direction: column;
        gap: 12px;
        padding: 12px;
        overflow-y: auto;
    }

    .center-column {
        order: 1;
    }

    .left-panel {
        min-height: 220px;
    }

    .right-panel {
        order: 2;
        width: 100%;
    }

    .control-panel {
        padding: 12px;
    }

    .control-main-layout {
        flex-direction: column;
        gap: 12px;
    }

    .control-info-column {
        flex: 1 1 auto;
        width: 100%;
    }

    .system-info {
        padding: 10px;
        margin-bottom: 10px;
    }

    .info-status,
    .info-material,
    .info-plate,
    .progress-header,
    .progress-percentage,
    .button-label,
    .control-switch-item .drop-mode-label,
    .control-switch-item .drop-mode-text {
        font-size: 14px;
    }

    .control-buttons {
        width: 100%;
        flex: 0 0 auto;
        grid-template-columns: repeat(4, minmax(0, 1fr));
        gap: 8px;
        overflow-x: hidden;
    }

    .control-switch-item {
        display: none;
    }

    .control-button {
        width: 100%;
        max-width: 88px;
    }

    .button-icon {
        font-size: 26px;
    }

    .control-switch-item {
        height: 86px;
    }

    .right-panel .truck-diagram {
        height: 150px;
        margin-bottom: 12px;
    }

    .data-grid {
        gap: 8px;
        margin-bottom: 0;
    }

    .data-value {
        font-size: 16px;
    }
}
</style>
