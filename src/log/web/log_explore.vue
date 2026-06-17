<template>
  <div class="industrial-panel log-page">
    <div class="toolbar">
      <div>
        <div class="panel-title">运行日志</div>
        <div class="panel-subtitle">实时记录状态机事件与状态转移</div>
      </div>
      <div class="header-actions">
        <el-button class="industrial-btn btn-warn" @click="clear_log">清除日志</el-button>
      </div>
    </div>
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

<style scoped>
@import "../../public/web/web_common/industrial_theme.css";

.log-page {
  height: 90%;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
}

.panel-subtitle {
  color: var(--text-sub);
  font-size: 13px;
}

.log-input {
  flex: 1;
  min-height: 0;
}

.log-input :deep(.el-textarea__inner) {
  height: 100%;
  resize: none;
  background: rgba(8, 28, 49, 0.6);
  border: 1px solid rgba(95, 169, 231, 0.35);
  color: #cceaff;
  font-family: monospace;
  font-size: 12px;
  border-radius: 6px;
}
</style>