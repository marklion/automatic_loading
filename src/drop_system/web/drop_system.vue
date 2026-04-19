<template>
    <div>
        <el-table :data="all_devices" style="width: 100%">
            <el-table-column prop="device_name" label="名称" />
            <el-table-column label="长度" prop="value" />
            <el-table-column label="开口率" prop="rate" />
            <el-table-column>
                <template #header>
                    <el-tag size="mini" v-if="system_on">自动放料</el-tag>
                    <el-tag size="mini" v-else type="danger">手动放料</el-tag>
                    <el-button type="success" text bg @click="turn_on_off">切换</el-button>
                </template>
                <template #default="scope">
                    <el-button type="success" size="mini" @click="oneTimeDrop(scope.row.device_name)">一次性放料</el-button>
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

<style></style>