<template>
    <div class="item-grid">
        <el-card class="item" v-for="(single_kit, index) in kits" :key="index">
            <template #header>
                {{ single_kit.kit_name }}
            </template>
            <div>
                <span>关联物料</span>
                <el-tag type="success" size="small">{{ single_kit.config_items['CONFIG_ITEM_CONFIG_KIT_STUFF_NAME']
                }}</el-tag>
                <el-button type="primary" size="small"
                    @click="change_stuff(single_kit.kit_name, single_kit.config_items['CONFIG_ITEM_CONFIG_KIT_STUFF_NAME'])">修改</el-button>
            </div>
        </el-card>
    </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, getCurrentInstance } from "vue";
const instance = getCurrentInstance();
const kits = ref([]);
let timer = null;
async function fetch_kits() {
    let resp = await instance.appContext.config.globalProperties.$call_remote_cli(
        "state_machine list_kits_json"
    );
    kits.value = resp;
}
async function change_stuff(kit_name, orig_stuff_name) {
    let new_stuff_name = await instance.appContext.config.globalProperties.$prompt(
        "请输入新的物料名称",
        {
            confirmButtonText: "确定",
            cancelButtonText: "取消",
            inputValue: orig_stuff_name,
        }
    );
    await instance.appContext.config.globalProperties.$call_remote_cli(
        `state_machine kit_config_item '${kit_name}' 'CONFIG_ITEM_CONFIG_KIT_STUFF_NAME' '${new_stuff_name.value}'`
    );
}
onMounted(async () => {
    await fetch_kits();
    timer = setInterval(async () => {
        await fetch_kits();
    }, 5000);
})
onUnmounted(() => {
    if (timer) {
        clearInterval(timer);
    }
})

</script>

<style scoped>
.item-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 12px;
}

.item {
    margin: 0;
}
</style>