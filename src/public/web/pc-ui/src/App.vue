<script setup>
import { useStatusInfo } from "@/stores/status_info";
import { DataSyncClient } from "./ws_sync_client";
const client = new DataSyncClient('/ws/');
client.watchData((key, data) => {
    if (key == 'status_info') {
        const statusInfoStore = useStatusInfo();
        Object.keys(data).forEach(key => {
            const processValue = (obj) => {
                Object.keys(obj).forEach(k => {
                    if (typeof obj[k] === 'number') {
                        obj[k] = Math.round(obj[k] * 100) / 100;
                    } else if (typeof obj[k] === 'object' && obj[k] !== null) {
                        processValue(obj[k]);
                    }
                });
            };
            processValue(data);
            if (typeof data[key] === 'number') {
                data[key] = Math.round(data[key] * 100) / 100;
            }
        });
        statusInfoStore.$patch(data);
    }
}, 'status_info');
client.watchData((key, data) => {
    if (key == 'sm_event') {
        const smEventStore = useSmEvent();
        smEventStore.setTo(data.to);
        smEventStore.setFrom(data.from);
        smEventStore.setEvent(data.event);
    }
}, 'sm_event');
</script>

<template>
    <RouterView></RouterView>
</template>

<style scoped></style>
