<template>
    <div>
        <el-table :data="all_devices" style="width: 100%">
            <el-table-column prop="device_name" label="名称" />
            <el-table-column label="长度" prop="value" />
            <el-table-column label="开口率" prop="rate" />
            <el-table-column label="操作" >
                <template #default="scope">
                    <el-button type="success" size="mini" @click="oneTimeDrop(scope.row.device_name)">一次性放料</el-button>
                </template>
            </el-table-column>
        </el-table>
    </div>
</template>

<script setup>
import { computed,getCurrentInstance } from "vue";
import { useStatusInfo } from "@/stores/status_info";

const instance = getCurrentInstance();
const status_info_store = useStatusInfo();
let all_devices = computed(() => {
    const devices = status_info_store.drop_system || [];
    return devices;
});

async function oneTimeDrop(deviceName) {
    await instance.appContext.config.globalProperties.$call_remote_cli(
        `drop_system one_time_drop "${deviceName}"`
    );
}
</script>

<style></style>