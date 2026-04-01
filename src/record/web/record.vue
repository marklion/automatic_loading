<template>
    <div>
        <el-date-picker v-model="dateRange" type="daterange" unlink-panels range-separator="至" start-placeholder="起始日期"
            end-placeholder="结束日期" :shortcuts="shortcuts" :size="size" />
        <el-button type="primary" @click="refresh">刷新</el-button>
        <el-table :data="all_record" style="width: 100%">
            <el-table-column prop="plate" label="车牌号" />
            <el-table-column prop="begin_time" label="开始时间" />
            <el-table-column prop="end_time" label="结束时间" />
            <el-table-column prop="dev_name" label="设备名称" />
            <el-table-column>
                <template #default="scope">
                    <el-button @click="play_record(scope.row.url)" type="success" size="small">查看视频</el-button>
                </template>
            </el-table-column>
        </el-table>
    </div>
</template>

<script setup>
import { onMounted, ref, getCurrentInstance } from "vue";
import moment from "@/my_moment.js";
const instance = getCurrentInstance();
const dateRange = ref('');
const all_record = ref([]);
const size = ref('default')
const shortcuts = [
    {
        text: '最近一周',
        value: () => {
            const end = new Date()
            const start = new Date()
            start.setTime(start.getTime() - 3600 * 1000 * 24 * 7)
            return [start, end]
        },
    },
    {
        text: '最近一月',
        value: () => {
            const end = new Date()
            const start = new Date()
            start.setTime(start.getTime() - 3600 * 1000 * 24 * 30)
            return [start, end]
        },
    },
    {
        text: '最近三月',
        value: () => {
            const end = new Date()
            const start = new Date()
            start.setTime(start.getTime() - 3600 * 1000 * 24 * 90)
            return [start, end]
        },
    },
]

async function refresh() {
    let begin_date = moment(dateRange.value[0]).format("YYYY-MM-DD");
    let end_date = moment(dateRange.value[1]).format("YYYY-MM-DD");
    let resp = await instance.appContext.config.globalProperties.$call_remote_cli(
        `record list_record * * '${begin_date}' '${end_date}' json`
    );
    all_record.value = resp;
}
function play_record(url) {
    window.open(url, '_blank');
}
onMounted(async () => {
    dateRange.value = [moment().subtract(7, 'days').toDate(), moment().toDate()];
    await refresh();
});

</script>

<style></style>