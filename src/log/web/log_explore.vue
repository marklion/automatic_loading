<template>
    <div>
        <el-button type="primary" @click="clear_log">清除日志</el-button>
        <el-input v-model="cur_logs" :autosize="{ minRows: 25, maxRows: 30 }" type="textarea" readonly />
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

<style lang="scss" scoped></style>