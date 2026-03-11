<template>
    <div>
        <el-table :data="all_devices" style="width: 100%">
            <el-table-column prop="device_name" label="名称" />
            <el-table-column label="长度" prop="value" />
            <el-table-column label="开口率" prop="rate" />
        </el-table>
    </div>
</template>

<script setup>
import { computed } from "vue";
import { useStatusInfo } from "@/stores/status_info";

const status_info_store = useStatusInfo();
let all_devices = computed(() => {
    const devices = status_info_store.drop_system || [];
    return devices;
});
</script>

<style></style>