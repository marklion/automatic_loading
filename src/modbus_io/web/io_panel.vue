<template>
  <div>
    <el-table :data="all_devices" style="width: 100%">
      <el-table-column prop="device_name" label="名称" />
      <el-table-column label="操作/状态">
        <template #default="scope">
          <div v-if="scope.row.is_output">
            <el-button v-if="scope.row.is_opened" @click="set_io(scope.row.device_name, false)"
              type="danger">松开</el-button>
            <el-button v-else @click="set_io(scope.row.device_name, true)" type="success">按下</el-button>
          </div>
          <div v-else>
            <el-icon v-if="scope.row.is_opened" color="green">
              <CircleCheckFilled />
            </el-icon>
            <el-icon v-else color="red">
              <CircleCloseFilled />
            </el-icon>
          </div>
        </template>
      </el-table-column>
    </el-table>
  </div>
</template>

<script setup>
import { onMounted, ref, getCurrentInstance } from "vue";
const instance = getCurrentInstance();
let all_devices = ref([]);
async function refresh_devices() {
  try {
    const res =
      await instance.appContext.config.globalProperties.$call_remote_cli(
        "modbus_io list_devices json"
      );
    all_devices.value = res;
  } catch (error) {
    console.log(error);
  }
  console.log(all_devices.value);
}
async function set_io(device_name, is_opened) {
  await instance.appContext.config.globalProperties.$call_remote_cli(
    `modbus_io device_operate "${device_name}" 1 ${is_opened ? 1 : 0}`
  );
  await refresh_devices();
}
onMounted(async () => {
  await refresh_devices();
  setInterval(async () => {
    await refresh_devices();
  }, 2000);
});
</script>

<style></style>
