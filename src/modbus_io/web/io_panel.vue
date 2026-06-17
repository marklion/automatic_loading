<template>
  <div class="industrial-panel io-page">
    <div class="toolbar">
      <div class="title-block">
        <div class="panel-title">按钮和指示灯</div>
        <div class="panel-subtitle">远程操作和监控按钮和指示灯</div>
      </div>
      <div class="header-actions">
        <el-button class="industrial-btn expand-btn" @click="open_operation_panel">展开操作</el-button>
      </div>
    </div>

    <el-dialog
      v-model="operation_dialog_visible"
      class="io-operation-dialog"
      :modal-class="'io-operation-dialog-overlay'"
      width="70vw"
      :append-to-body="true"
    >
      <template #header>
        <div class="dialog-header">
          <div>
            <div class="panel-title">按钮和指示灯</div>
            <div class="panel-subtitle">远程操作和监控按钮和指示灯</div>
          </div>
          <div class="header-actions">
            <el-button class="industrial-btn" @click="pump_control(true)">开油泵</el-button>
            <el-button class="industrial-btn btn-warn" @click="pump_control(false)">关油泵</el-button>
          </div>
        </div>
      </template>

      <div class="dialog-content">
        <div class="summary-row">
          <div class="summary-chip">输入设备 {{ input_devices.length }}</div>
          <div class="summary-chip">输出设备 {{ output_devices.length }}</div>
          <div class="summary-chip">亮灯 {{ active_outputs }}</div>
        </div>

        <div class="io-columns">
          <section class="io-section">
            <div class="section-title">输入设备（操作）</div>
            <div v-if="input_devices.length === 0" class="empty-tip">暂无输入设备</div>
            <div v-else class="device-grid input-grid">
              <div v-for="device in input_devices" :key="`in-${device.device_name}`" class="device-card">
                <div class="device-name">{{ device.device_name }}</div>
                <el-button
                  v-if="device.is_opened"
                  class="industrial-btn btn-warn"
                  @click="set_io(device.device_name, false)"
                >松开</el-button>
                <el-button
                  v-else
                  class="industrial-btn"
                  @click="set_io(device.device_name, true)"
                >按下</el-button>
              </div>
            </div>
          </section>

          <section class="io-section">
            <div class="section-title">输出设备（监控）</div>
            <div v-if="output_devices.length === 0" class="empty-tip">暂无输出设备</div>
            <div v-else class="device-grid lamp-grid">
              <div v-for="device in output_devices" :key="`out-${device.device_name}`" class="device-card lamp-card">
                <div class="lamp-row">
                  <span class="status-lamp" :class="device.is_opened ? 'status-on' : 'status-off'"></span>
                </div>
                <div class="device-name">{{ device.device_name }}</div>
              </div>
            </div>
          </section>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup>
import { computed, getCurrentInstance, ref } from "vue";
import { useStatusInfo } from "@/stores/status_info";
const instance = getCurrentInstance();
const operation_dialog_visible = ref(false);
const status_info_store = useStatusInfo();
let all_devices = computed(() => {
  const devices = status_info_store.modbus_io || [];
  return devices;
});
const input_devices = computed(() => {
  return all_devices.value.filter((device) => device.is_output);
});
const output_devices = computed(() => {
  return all_devices.value.filter((device) => !device.is_output);
});
const active_outputs = computed(() => {
  return output_devices.value.filter((device) => device.is_opened).length;
});

async function set_io(device_name, is_opened) {
  await instance.appContext.config.globalProperties.$call_remote_cli(
    `modbus_io device_operate "${device_name}" 1 ${is_opened ? 1 : 0}`
  );
}

async function pump_control(turn_on) {
  await instance.appContext.config.globalProperties.$call_remote_cli(
    `modbus_io pump_control ${turn_on ? 1 : 0}`
  );
}

function open_operation_panel() {
  operation_dialog_visible.value = true;
}

</script>

<style scoped>
@import "../../public/web/web_common/industrial_theme.css";

.io-page {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 6px;
}

.title-block {
  min-width: 0;
}

.header-actions {
  display: flex;
  gap: 8px;
  flex: 0 0 auto;
}

.panel-subtitle {
  color: var(--text-sub);
  font-size: 13px;
}

.expand-btn {
  min-height: 26px;
  padding: 3px 10px;
}

.dialog-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 10px;
  flex-wrap: wrap;
}

.dialog-header .panel-title {
  color: #ecf8ff;
}

.dialog-header .panel-subtitle {
  color: #cfe9ff;
}

.dialog-content {
  height: 70vh;
  min-height: 320px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  overflow: auto;
}

:global(.el-dialog.io-operation-dialog) {
  background:
    radial-gradient(circle at 85% -10%, rgba(117, 199, 255, 0.18), transparent 45%),
    linear-gradient(135deg, #0b213a, #12385b);
  border: 1px solid rgba(95, 169, 231, 0.35);
  box-shadow: inset 0 0 0 1px rgba(110, 189, 247, 0.1), 0 8px 24px rgba(1, 10, 20, 0.3);
}

:global(.el-dialog.io-operation-dialog .el-dialog__header) {
  margin-right: 0;
  padding-bottom: 8px;
  border-bottom: 1px solid rgba(95, 169, 231, 0.28);
}

:global(.el-dialog.io-operation-dialog .el-dialog__body) {
  background: transparent;
  padding-top: 10px;
}

:global(.el-dialog.io-operation-dialog .el-dialog__title) {
  color: #d9efff;
}

:global(.io-operation-dialog-overlay) {
  background: rgba(2, 12, 23, 0.55);
}

.summary-row {
  display: flex;
  gap: 6px;
  flex-wrap: wrap;
}

.header-actions {
  display: flex;
  gap: 8px;
  flex: 0 0 auto;
}

.summary-chip {
  border: 1px solid rgba(95, 169, 231, 0.5);
  background: rgba(33, 96, 149, 0.28);
  color: #cceaff;
  border-radius: 999px;
  padding: 4px 10px;
  font-size: 12px;
}

.io-columns {
  display: grid;
  grid-template-columns: 1.35fr 1fr;
  gap: 8px;
  min-height: 0;
}

.io-section {
  border: 1px solid rgba(95, 169, 231, 0.35);
  background: rgba(8, 28, 49, 0.38);
  border-radius: 8px;
  padding: 8px;
}

.section-title {
  margin-bottom: 8px;
  font-size: 13px;
  color: #bfe3ff;
  font-weight: 700;
}

.device-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
  gap: 8px;
}

.input-grid {
  grid-template-columns: repeat(auto-fill, minmax(110px, 1fr));
}

.lamp-grid {
  grid-template-columns: repeat(6, 1fr);
}

.device-card {
  border: 1px solid rgba(95, 169, 231, 0.3);
  background: rgba(16, 44, 72, 0.45);
  border-radius: 6px;
  padding: 7px;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.input-grid .device-card {
  padding: 6px;
  gap: 5px;
}

.lamp-card {
  align-items: flex-start;
}

.device-name {
  color: #e0f4ff;
  font-size: 12px;
  line-height: 1.25;
  word-break: break-all;
}

.lamp-row {
  display: flex;
  align-items: center;
  justify-content: center;
}

.status-lamp {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  display: inline-block;
  box-shadow: 0 0 0 1px rgba(230, 244, 255, 0.2), 0 0 8px rgba(0, 0, 0, 0.35) inset;
}

:deep(.device-card .el-button) {
  min-height: 28px;
  padding: 4px 10px;
}

:deep(.input-grid .device-card .el-button) {
  width: 100%;
  min-height: 26px;
  padding: 3px 8px;
}

.status-lamp.status-on {
  background: #2dd36f;
  box-shadow: 0 0 0 1px rgba(173, 255, 208, 0.35), 0 0 9px rgba(45, 211, 111, 0.6);
}

.status-lamp.status-off {
  background: #f04444;
  box-shadow: 0 0 0 1px rgba(255, 187, 187, 0.3), 0 0 9px rgba(240, 68, 68, 0.55);
}

.empty-tip {
  color: #8ab5d6;
  font-size: 12px;
}

@media (max-width: 960px) {
  .dialog-header {
    align-items: flex-start;
  }

  .dialog-content {
    height: 70vh;
  }

  .io-columns {
    grid-template-columns: 1fr;
  }

  .input-grid {
    grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));
  }
}
</style>
