<template>
    <div class="scale-container">
        <el-statistic title="地磅读数" :value="current_scale" :precision="2"></el-statistic>
    </div>
</template>

<script setup>
import { computed } from 'vue'
import { useStatusInfo } from '@/stores/status_info'
const statusInfo = useStatusInfo();
const current_scale = computed(() => statusInfo.scale.weight || 0);
</script>

<style lang="scss" scoped>
.scale-container {
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100%;
}
:deep(.el-statistic__head) {
    font-size: 18px;
    font-weight: bold;
}

</style>