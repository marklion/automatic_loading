<template>
  <div>
    <el-input v-model="remote_hostname" placeholder="Please input">
      <template #append>
        <el-button @click="enter_config_page" type="primary">进入配置界面</el-button>
      </template>
    </el-input>
    <el-dialog v-model="config_page_should_show" title="配置">
      <iframe
        src="/wetty"
        style="width: 100%; height: 500px; border: none"
      ></iframe>
    </el-dialog>
    <IoPanel> </IoPanel>
  </div>
</template>

<script setup>
import IoPanel from "../../../../../modbus_io/web/io_panel.vue";
import { ref, computed } from "vue";
import { useRemoteHostName } from "@/stores/remote_name";
const hostname_store = useRemoteHostName();
const remote_hostname = computed({
  get: () => hostname_store.remoteName,
  set: (value) => hostname_store.setRemoteName(value),
});
const config_page_should_show = ref(false);
function enter_config_page() {
  config_page_should_show.value = true;
}
</script>

<style></style>
