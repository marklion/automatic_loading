<script setup>
import { useStatusInfo } from "@/stores/status_info";
import { DataSyncClient } from "./ws_sync_client";
const client = new DataSyncClient('/ws/');
client.watchData((key, data) => {
    if (key == 'status_info') {
        const statusInfoStore = useStatusInfo();
        statusInfoStore.$patch(data);
    }
}, 'status_info');
</script>

<template>
    <RouterView></RouterView>
</template>

<style scoped></style>
