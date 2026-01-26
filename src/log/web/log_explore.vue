<template>
    <div class="log-container">
        <el-button type="primary" @click="clear_log">清除日志</el-button>
        <el-input v-model="cur_logs" type="textarea" readonly class="log-input" />
    </div>
</template>

<script setup>
import { ref, computed, watch } from "vue";
import { useSmEvent } from "@/stores/sm_event";
import moment from "@/my_moment.js";
const smEventStore = useSmEvent();
const cur_logs = ref("");
const additional_logs = computed(() => {
    let time = moment().format("MM-DD HH:mm:ss");
    let ret = `${time}-> ${smEventStore.event} : 从[${smEventStore.from}]到[${smEventStore.to}]\n`;
    return ret;
});
watch(additional_logs, (new_val) => {
    cur_logs.value += new_val;
});

async function clear_log() {
    cur_logs.value = "";
}
</script>

<style lang="scss" scoped>
.log-container {
    display: flex;
    flex-direction: column;
    height: 100%;
}

.log-input {
    flex: 1;
    min-height: 0; // allow textarea to shrink within flex column
}

.log-input :deep(.el-textarea__inner) {
    height: 100%;
    resize: none;
}
</style>