<template>
    <div>
        <el-date-picker v-model="dateRange" type="daterange" unlink-panels range-separator="至" start-placeholder="起始日期"
            end-placeholder="结束日期" :shortcuts="shortcuts" size="default" :clearable="false" @change="refresh" />
        <el-button type="primary" @click="refresh">刷新</el-button>
        <el-table :data="all_record" style="width: 100%">
            <el-table-column type="index" label="序号" :index="1" min-width="15" />
            <el-table-column prop="plate" label="车牌号" />
            <el-table-column prop="begin_time" label="开始时间" />
            <el-table-column prop="end_time" label="结束时间" />
            <el-table-column prop="dev_name" label="设备名称" />
            <el-table-column prop="load" label="载重" />
            <el-table-column :label="`平均耗时${ava_spend}分钟`">
                <template #default="scope">
                    <span>
                        {{ ((new Date(scope.row.end_time) - new Date(scope.row.begin_time)) / 1000 / 60).toFixed(2) }}分钟
                    </span>
                    <span v-if="is_justified(scope.row)">(干预)</span>
                </template>
            </el-table-column>
            <el-table-column :label="'共' + all_record.length + '条记录' + `(自动率:${auto_rate})`">
                <template #default="scope">
                    <el-button @click="play_record(scope.row.url)" type="success" size="small">查看视频</el-button>
                    <el-button @click="download_video(scope.$index)" type="primary" size="small">生成视频</el-button>
                    <a v-if="scope.row.video_file_name && scope.row.video_download_progress == 100"
                        :href="'/video/' + scope.row.video_file_name" download>下载</a>
                </template>
            </el-table-column>
        </el-table>
    </div>
</template>

<script setup>
import { onMounted, ref, getCurrentInstance, computed } from "vue";
import moment from "@/my_moment.js";
const instance = getCurrentInstance();
const dateRange = ref('');
const all_record = ref([]);
const auto_rate = computed(() => {
    if (all_record.value.length == 0) {
        return '0.00%';
    }
    let justified_count = all_record.value.filter(r => r.justified).length;
    return ((all_record.value.length - justified_count) / all_record.value.length * 100).toFixed(2) + '%';
});
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
function is_justified(record) {
    return record.justified;
}
const ava_spend = computed(() => {
    if (all_record.value.length == 0) {
        return '0.00';
    }
    let total_spend = all_record.value.reduce((sum, r) => {
        return sum + (new Date(r.end_time) - new Date(r.begin_time)) / 1000 / 60;
    }, 0);
    return (total_spend / all_record.value.length).toFixed(2);
});
async function download_video(index) {
    let begin_date = moment(dateRange.value[0]).format("YYYY-MM-DD");
    let end_date = moment(dateRange.value[1]).format("YYYY-MM-DD");
    await instance.appContext.config.globalProperties.$call_remote_cli(
        `record make_video ${index} * * ${begin_date} ${end_date}`
    );
    await refresh();
}
onMounted(async () => {
    dateRange.value = [moment().toDate(), moment().toDate()];
    await refresh();
});

</script>

<style></style>