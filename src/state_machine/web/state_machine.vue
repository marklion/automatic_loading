<template>
    <div v-if="should_show" class="industrial-panel state-machine-page">
        <div class="toolbar">
            <div>
                <div class="panel-title">运行状态</div>
                <div class="panel-subtitle">状态机运行信息、定位区间与装载趋势监控</div>
            </div>
            <div class="header-actions action-wrap">
                <el-button v-if="!isRecording" class="industrial-btn" @click="my_startAudioStreaming">广播</el-button>
                <el-button v-else class="industrial-btn btn-warn" @click="my_stopAudioProcessing">停止</el-button>
                <el-button class="industrial-btn btn-outline" @click="enter_manual">手动</el-button>
                <el-button class="industrial-btn btn-warn" @click="emergencyStop">急停</el-button>
                <el-button class="industrial-btn" @click="resetStateMachine">重置</el-button>
            </div>
        </div>

        <div class="summary-row">
            <div class="summary-chip">当前状态 {{ sm_status.status || '-' }}</div>
            <div class="summary-chip">工作溜槽 {{ sm_status.is_front_dropped ? '前' : '后' }}</div>
            <div class="summary-chip">车牌 {{ sm_status.vehicle_info?.plate || '-' }}</div>
            <div class="summary-chip">物料 {{ sm_status.vehicle_info?.stuff_name || '-' }}</div>
            <div class="summary-chip">
                <span>应用配置套件 {{ sm_status.applied_kit || '-' }}</span>
                <el-button class="chip-detail-btn" @click="show_kit_detail = true">详细</el-button>
            </div>
        </div>

        <div class="progress-grid">
            <section class="progress-panel progress-panel-stack">
                <div class="progress-head">
                    <div class="section-title progress-title">车厢前壁位置</div>
                    <div class="progress-value-inline">
                        {{ sm_status.basic_config.front_min_x }} - {{ sm_status.vehicle_front_x }} - {{ sm_status.basic_config.front_max_x }}
                    </div>
                </div>
                <el-progress
                    class="thin-progress success-progress"
                    status="success"
                    :show-text="false"
                    :stroke-width="12"
                    :percentage="calcu_percentage(sm_status.vehicle_front_x, sm_status.basic_config.front_min_x, sm_status.basic_config.front_max_x)"
                />

                <div class="progress-head">
                    <div class="section-title progress-title">车厢后壁位置</div>
                    <div class="progress-value-inline">
                        {{ sm_status.basic_config.tail_min_x }} - {{ sm_status.vehicle_tail_x }} - {{ sm_status.basic_config.tail_max_x }}
                    </div>
                </div>
                <el-progress
                    class="thin-progress warn-progress"
                    status="warning"
                    :show-text="false"
                    :stroke-width="12"
                    :percentage="calcu_percentage(sm_status.vehicle_tail_x, sm_status.basic_config.tail_min_x, sm_status.basic_config.tail_max_x)"
                />
            </section>

            <section class="progress-panel dashboard-panel">
                <div class="progress-head">
                    <div class="section-title progress-title">地磅示数</div>
                    <div class="progress-value-inline">
                        25 - {{ sm_status.current_load }} - 50
                    </div>
                </div>
                <el-progress
                    class="thin-progress success-progress"
                    status="success"
                    :show-text="false"
                    :stroke-width="12"
                    :percentage="calcu_percentage(sm_status.current_load, 25, 50)"
                />

                <div class="progress-head">
                    <div class="section-title progress-title">料堆满度</div>
                    <div class="progress-value-inline">
                        -1.2 - {{ sm_status.stuff_full_offset }} - 0
                    </div>
                </div>
                <el-progress
                    class="thin-progress warn-progress"
                    status="warning"
                    :show-text="false"
                    :stroke-width="12"
                    :percentage="calcu_percentage(sm_status.stuff_full_offset, -1.2, 0)"
                />
            </section>
        </div>

        <el-dialog :append-to-body="true" v-model="show_kit_detail" :show-close="true" width="500">
            <kit-stuff></kit-stuff>
        </el-dialog>
    </div>
</template>

<script setup>
import { computed, getCurrentInstance, ref } from "vue";
import { useStatusInfo } from "@/stores/status_info";
import { startAudioStreaming, stopAudioProcessing } from "@/broadcast";
import KitStuff from "./kit_stuff.vue";
const instance = getCurrentInstance();
const isRecording = ref(false)
const show_kit_detail = ref(false);

async function my_startAudioStreaming() {
    isRecording.value = true;
    await startAudioStreaming();
}
async function my_stopAudioProcessing() {
    isRecording.value = false;
    await stopAudioProcessing();
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
@import "../../public/web/web_common/industrial_theme.css";

.state-machine-page {
    display: flex;
    flex-direction: column;
    gap: 10px;
}

.toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
}

.panel-subtitle {
    color: var(--text-sub);
    font-size: 13px;
}

.action-wrap {
    flex-wrap: wrap;
    justify-content: flex-end;
}

.summary-row {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
}

.summary-chip {
    border: 1px solid rgba(95, 169, 231, 0.5);
    background: rgba(33, 96, 149, 0.28);
    color: #cceaff;
    border-radius: 999px;
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 2px 9px;
    font-size: 12px;
    line-height: 1;
}

.summary-chip > span {
    display: inline-flex;
    align-items: center;
    line-height: 1;
}

.chip-detail-btn {
    min-height: auto;
    padding: 0;
    font-size: 12px;
    line-height: 1.1;
    color: #bfe3ff;
    background: transparent;
    border: 0;
    text-decoration: underline;
    text-underline-offset: 2px;
    box-shadow: none;
}

.chip-detail-btn:hover,
.chip-detail-btn:focus {
    color: #ecf8ff;
    background: transparent;
    border: 0;
}

.chip-detail-btn:active {
    color: #ffffff;
}

:deep(.chip-detail-btn.el-button) {
    margin: 0;
}

:deep(.chip-detail-btn.el-button span) {
    text-decoration: inherit;
}

.progress-grid {
    display: grid;
    grid-template-columns: 1.05fr 1.2fr;
    gap: 8px;
}

.progress-panel {
    border: 1px solid rgba(95, 169, 231, 0.35);
    background: rgba(8, 28, 49, 0.38);
    border-radius: 8px;
    padding: 10px;
}

.progress-panel-stack {
    display: flex;
    flex-direction: column;
}

.progress-head {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    gap: 10px;
}

.progress-title {
    margin-bottom: 0;
    flex: 0 0 auto;
}

.progress-value-inline {
    flex: 0 0 auto;
    color: #e0f4ff;
    font-size: 12px;
    line-height: 1.15;
    white-space: nowrap;
    text-align: right;
}

.section-title {
    margin-bottom: 10px;
    font-size: 13px;
    color: #bfe3ff;
    font-weight: 700;
}

.dashboard-panel {
    min-width: 0;
}

.load-progress-stack {
    display: flex;
    flex-direction: column;
    gap: 10px;
}

.progress-item {
    display: flex;
    flex-direction: column;
    gap: 6px;
}

.thin-progress {
    width: 100%;
}

:deep(.thin-progress .el-progress-bar__outer) {
    height: 12px !important;
    border-radius: 999px;
}

:deep(.thin-progress .el-progress-bar__inner) {
    border-radius: 999px;
}

:deep(.thin-progress .el-progress__text) {
    line-height: 1.2;
}

.range-caption {
    color: #8ab5d6;
    font-size: 12px;
    line-height: 1.15;
}

:deep(.el-progress-bar__outer) {
    background: rgba(9, 33, 57, 0.85);
}

:deep(.el-progress-bar__innerText) {
    color: #ecf8ff;
}

:deep(.el-progress__text) {
    color: #e0f4ff;
}

:deep(.el-dialog) {
    background: linear-gradient(135deg, #0b213a, #12385b);
    border: 1px solid rgba(95, 169, 231, 0.35);
}

:deep(.el-dialog__title) {
    color: #d9efff;
}

@media (max-width: 1080px) {
    .progress-grid {
        grid-template-columns: 1fr;
    }
}

.action-wrap {
    flex-wrap: wrap;
    justify-content: flex-end;
}

.summary-row {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
}

.summary-chip {
    border: 1px solid rgba(95, 169, 231, 0.5);
    background: rgba(33, 96, 149, 0.28);
    color: #cceaff;
    border-radius: 999px;
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 2px 9px;
    font-size: 12px;
    line-height: 1;
}

.summary-chip > span {
    display: inline-flex;
    align-items: center;
    line-height: 1;
}

.chip-detail-btn {
    min-height: auto;
    padding: 0;
    font-size: 12px;
    line-height: 1.1;
    color: #bfe3ff;
    background: transparent;
    border: 0;
    text-decoration: underline;
    text-underline-offset: 2px;
    box-shadow: none;
}

.chip-detail-btn:hover,
.chip-detail-btn:focus {
    color: #ecf8ff;
    background: transparent;
    border: 0;
}

.chip-detail-btn:active {
    color: #ffffff;
}

:deep(.chip-detail-btn.el-button) {
    margin: 0;
}

:deep(.chip-detail-btn.el-button span) {
    text-decoration: inherit;
}

.progress-grid {
    display: grid;
    grid-template-columns: 1.05fr 1.2fr;
    gap: 8px;
}

.progress-panel {
    border: 1px solid rgba(95, 169, 231, 0.35);
    background: rgba(8, 28, 49, 0.38);
    border-radius: 8px;
    padding: 10px;
}

.progress-panel-stack {
    display: flex;
    flex-direction: column;
}

.progress-head {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    gap: 10px;
}

.progress-title {
    margin-bottom: 0;
    flex: 0 0 auto;
}

.progress-value-inline {
    flex: 0 0 auto;
    color: #e0f4ff;
    font-size: 12px;
    line-height: 1.15;
    white-space: nowrap;
    text-align: right;
}

.stacked-gap {
    margin-top: 10px;
}

.section-title {
    margin-bottom: 10px;
    font-size: 13px;
    color: #bfe3ff;
    font-weight: 700;
}

.dashboard-panel {
    min-width: 0;
}

.vertical-progress-list {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 14px;
}

.vertical-progress-item {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
}

.vertical-progress-top {
    width: 100%;
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    gap: 10px;
}

.vertical-meter-wrap {
    width: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
}

.vertical-meter-track {
    width: 36px;
    height: 150px;
    border-radius: 18px;
    border: 1px solid rgba(95, 169, 231, 0.42);
    background: linear-gradient(180deg, rgba(9, 33, 57, 0.92), rgba(10, 40, 70, 0.78));
    box-shadow: inset 0 0 0 1px rgba(110, 189, 247, 0.08);
    padding: 3px;
    display: flex;
    align-items: flex-end;
    overflow: hidden;
}

.vertical-meter-fill {
    width: 100%;
    border-radius: 14px;
    min-height: 2px;
}

.load-fill {
    background: linear-gradient(180deg, #75d6a2 0%, #2dd36f 100%);
    box-shadow: 0 0 10px rgba(45, 211, 111, 0.55);
}

.offset-fill {
    background: linear-gradient(180deg, #ffd18a 0%, #f0a43f 100%);
    box-shadow: 0 0 10px rgba(240, 164, 63, 0.45);
}

.vertical-meter-scale {
    width: 36px;
    display: flex;
    justify-content: space-between;
    color: #8ab5d6;
    font-size: 12px;
    line-height: 1;
}

.vertical-meter-scale span {
    transform: translateY(-1px);
}

.dashboard-grid {
    display: none;
}

:deep(.el-progress-bar__outer) {
    background: rgba(9, 33, 57, 0.85);
}

:deep(.el-progress-bar__innerText) {
    color: #ecf8ff;
}

:deep(.el-progress-dashboard__track) {
    stroke: rgba(62, 143, 209, 0.3);
}

:deep(.el-progress__text) {
    color: #e0f4ff;
}

:deep(.el-dialog) {
    background: linear-gradient(135deg, #0b213a, #12385b);
    border: 1px solid rgba(95, 169, 231, 0.35);
}

:deep(.el-dialog__title) {
    color: #d9efff;
}

@media (max-width: 1080px) {
    .progress-grid {
        grid-template-columns: 1fr;
    }
}
</style>
