<template>
    <div>
        <h2>料堆高度</h2>
        <el-progress type="dashboard" status="exception" :percentage="calcu_percentage(head_offset, -1, 0.3)">
            <template #default="{ percentage }">
                <div>{{ head_offset }}</div>
                <div>前溜槽下料满度</div>
            </template>
        </el-progress>
        <el-progress type="dashboard" status="exception" :percentage="calcu_percentage(tail_offset, -1, 0.3)">
            <template #default="{ percentage }">
                <div>{{ tail_offset }}</div>
                <div>后溜槽下料满度</div>
            </template>
        </el-progress>
    </div>
</template>

<script setup>
import { onMounted, ref, getCurrentInstance } from "vue";
const instance = getCurrentInstance();
const head_offset = ref(0);
const tail_offset = ref(0);
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
async function fetch_offsets() {
    try {
        let resp = await instance.appContext.config.globalProperties.$call_remote_cli(
            "xlrd read_offset 0"
        );
        head_offset.value = resp.offset;
        resp = await instance.appContext.config.globalProperties.$call_remote_cli(
            "xlrd read_offset 1"
        );
        tail_offset.value = resp.offset;
    } catch (error) {

    }
}
onMounted(() => {
    setInterval(async () => {
        await fetch_offsets();
    }, 2000);
});
</script>

<style lang="scss" scoped></style>