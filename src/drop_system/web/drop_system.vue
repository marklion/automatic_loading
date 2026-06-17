<template>
    <div class="industrial-panel">
        <div>
            <div class="panel-title">放料控制台</div>
            <div class="panel-subtitle">监控放料口闭环控制</div>
        </div>

        <el-table :data="all_devices" style="width: 100%" class="industrial-table">
            <el-table-column prop="device_name" label="名称" />
            <el-table-column label="长度" prop="value" width="90" />
            <el-table-column label="开口率" prop="rate" width="90" />
            <el-table-column>
                <template #header>
                    <div class="header-actions">
                        <el-tag size="small" v-if="system_on" class="status-tag">自动放料</el-tag>
                        <el-tag size="small" v-else class="status-tag status-tag-off">手动放料</el-tag>
                        <el-button class="industrial-btn btn-outline switch-btn" @click="turn_on_off">切换</el-button>
                    </div>
                </template>
                <template #default="scope">
                    <el-button class="industrial-btn" @click="oneTimeDrop(scope.row.device_name)">一次性放料</el-button>
                </template>
            </el-table-column>
        </el-table>
    </div>
</template>

<script setup>
import { computed, getCurrentInstance } from "vue";
import { useStatusInfo } from "@/stores/status_info";

const instance = getCurrentInstance();
const status_info_store = useStatusInfo();
let all_devices = computed(() => {
    const devices = status_info_store.drop_system?.devices || [];
    return devices;
});
let system_on = computed(() => {
    return status_info_store.drop_system?.system_on || false;
});

async function oneTimeDrop(deviceName) {
    await instance.appContext.config.globalProperties.$call_remote_cli(
        `drop_system one_time_drop "${deviceName}"`
    );
}
async function turn_on_off() {
    await instance.appContext.config.globalProperties.$call_remote_cli(
        `drop_system turn_on_off "${system_on.value ? "off" : "on"}"`
    );
}
</script>

<style scoped>
@import "../../public/web/web_common/industrial_theme.css";

.demo-section {
    margin-top: 14px;
    padding-top: 12px;
    border-top: 1px dashed rgba(110, 181, 235, 0.35);
}

.demo-title {
    margin-bottom: 8px;
    color: var(--text-sub);
    font-size: 13px;
}

.demo-buttons {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
}

:deep(.switch-btn.el-button) {
    min-height: 24px;
    padding: 2px 8px;
    font-size: 12px;
}
</style>