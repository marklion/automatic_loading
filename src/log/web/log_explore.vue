<template>
    <div>
        <el-button type="primary" @click="clear_log">清除日志</el-button>
        <el-input v-model="cur_logs" :rows="30" type="textarea" readonly />
    </div>
</template>

<script setup>
import { onMounted, ref, getCurrentInstance } from "vue";
const instance = getCurrentInstance();
const cur_logs = ref("");
const cur_log_index = ref(0);
async function get_cur_log_index() {
    let ret = 0;
    try {
        let resp = await instance.appContext.config.globalProperties.$call_remote_cli(
            "log show_log_line_num 1 2"
        );
        ret = resp.log_line_num;
    } catch (error) {

    }
    return ret;
}
async function clear_log() {
    cur_log_index.value = await get_cur_log_index();
    cur_logs.value = "";
}
async function fetch_logs() {
    try {
        let resp = await instance.appContext.config.globalProperties.$call_remote_cli(
            `log show_logs 1 2 ${cur_log_index.value}`
        );
        for (let line of resp) {
            line = line.split("|")[0] + "|" + line.split("|")[1] + "|" + line.split("|")[3].split("[INFO]:")[1];
            cur_logs.value += line + "\n";
        }
        cur_log_index.value += resp.length;
        let new_index = await get_cur_log_index();
        if (cur_log_index.value > new_index) {
            cur_log_index.value = new_index;
        }
    } catch (error) {
    }
}
onMounted(async () => {
    await clear_log();
    setInterval(async () => {
        await fetch_logs();
    }, 2000);
});
</script>

<style lang="scss" scoped></style>