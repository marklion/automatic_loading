<template>
    <div v-if="should_show">
        <el-descriptions title="运行状态" :column="3" border>
            <template #extra>
                <el-button type="warning" @click="enter_manual">手动</el-button>
                <el-button type="danger" @click="emergencyStop">急停</el-button>
                <el-button type="primary" @click="resetStateMachine">重置</el-button>
            </template>
            <el-descriptions-item label="当前状态">
                <el-tag type="primary">{{ sm_status.status }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="车牌号">
                {{ sm_status.vehicle_info.plate }}
            </el-descriptions-item>
            <el-descriptions-item label="物料">
                {{ sm_status.vehicle_info.stuff_name }}
            </el-descriptions-item>
        </el-descriptions>
        <el-row align="middle">
            <el-col :span="8">
                <el-progress status="success" :text-inside="true" :stroke-width="20"
                    :percentage="calcu_percentage(sm_status.vehicle_front_x, sm_status.basic_config.front_min_x, sm_status.basic_config.front_max_x)">
                    <template #default="{ percentage }">
                        <div class="label-text">{{ sm_status.basic_config.front_min_x }} - {{ sm_status.vehicle_front_x
                        }} - {{
                                sm_status.basic_config.front_max_x }}</div>
                    </template>
                </el-progress>
                <div class="label-text">车厢前壁位置</div>
            </el-col>
            <el-col :span="8">
                <el-progress status="warning" :text-inside="true" :stroke-width="20"
                    :percentage="calcu_percentage(sm_status.vehicle_tail_x, sm_status.basic_config.tail_min_x, sm_status.basic_config.tail_max_x)">
                    <template #default="{ percentage }">
                        <div class="label-text">{{ sm_status.basic_config.tail_min_x }} - {{ sm_status.vehicle_tail_x }}
                            - {{
                                sm_status.basic_config.tail_max_x }}</div>
                    </template>
                </el-progress>
                <div class="label-text">车厢后壁位置</div>
            </el-col>
            <el-col :span="8">
                <el-progress type="dashboard"
                    :percentage="calcu_percentage(sm_status.current_load, make_range_by_max(sm_status.basic_config.max_load).min, make_range_by_max(sm_status.basic_config.max_load).max)">
                    <template #default="{ percentage }">
                        <div>{{ sm_status.current_load }}<->{{ sm_status.basic_config.max_load }}</div>
                        <div>地磅示数</div>
                    </template>
                </el-progress>
                <el-progress type="dashboard"
                    :percentage="calcu_percentage(sm_status.stuff_full_offset, make_range_by_max(sm_status.basic_config.max_full_offset, -4).min, make_range_by_max(sm_status.basic_config.max_full_offset, -4).max)">
                    <template #default="{ percentage }">
                        <div>{{ sm_status.stuff_full_offset }}<->{{ sm_status.basic_config.max_full_offset }}</div>
                        <div>料堆满度</div>
                    </template>
                </el-progress>
            </el-col>
        </el-row>
    </div>
</template>

<script setup>
import { onMounted, ref, getCurrentInstance } from "vue";
const should_show = ref(false);
const instance = getCurrentInstance();
const sm_status = ref({});

function make_range_by_max(max_value, min_value = 0) {
    let ret = {
        min: min_value,
        max: max_value / 0.8,
    }
    return ret;
}
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
async function emergencyStop() {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt e"
        );
    } catch (error) {
        console.log(error);
    }
}
async function resetStateMachine() {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt r"
        );
    } catch (error) {
        console.log(error);
    }
}
async function refresh_status() {
    try {
        const res =
            await instance.appContext.config.globalProperties.$call_remote_cli(
                "state_machine show_status json"
            );
        sm_status.value = res;
        should_show.value = true;
    } catch (error) {
        console.log(error);
    }
    console.log(sm_status.value);
}
async function enter_manual() {
    try {
        await instance.appContext.config.globalProperties.$call_remote_cli(
            "state_machine sm_opt m"
        );
    } catch (error) {
        console.log(error);
    }
}
onMounted(async () => {
    setInterval(async () => {
        await refresh_status();
    }, 1000);
});

</script>

<style lang="scss" scoped>
.label-text {
    text-align: center;
    font-size: 12px;
    color: red;
}
</style>
