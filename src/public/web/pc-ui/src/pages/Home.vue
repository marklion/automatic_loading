<template>
  <div>
    <el-input v-model="remote_hostname" placeholder="Please input">
      <template #append>
        <el-button @click="enter_config_page" type="primary">进入配置界面</el-button>
      </template>
      <template #prepend>
        <el-switch v-model="resize_switch" active-text="允许调整大小" inactive-text="禁止调整大小"></el-switch>
      </template>
      <template #prefix v-if="resize_switch">
        <el-button @click="reset_layout">重置布局</el-button>
      </template>
    </el-input>
    <el-dialog v-model="config_page_should_show" :z-index="2000" title="配置" fullscreen>
      <iframe src="/wetty" style="width: 100vw; height: 100vh; border: none"></iframe>
    </el-dialog>
    <grid-layout :layout="layout" :col-num="12" :row-height="30" :is-draggable="resize_switch"
      :is-resizable="resize_switch" :auto-size="false" @layout-updated="saveLayout">
      <grid-item v-for="item in layout" :key="item.i" :x="item.x" :y="item.y" :w="item.w" :h="item.h" :i="item.i">
        <!-- 你的组件内容 -->
        <div>
          <component :is="my_components[item.i]"></component>
        </div>
      </grid-item>
    </grid-layout>
  </div>
</template>

<script setup>
import IoPanel from "../../../../../modbus_io/web/io_panel.vue";
import StateMachine from "../../../../../state_machine/web/state_machine.vue";
import LogExplore from "../../../../../log/web/log_explore.vue";
import XlrdShow from "../../../../../xlrd/web/xlrd_show.vue";
import LiveCamera from "../../../../../live_camera/web/live_camera.vue";
import { ref, computed, onMounted } from "vue";
import { useRemoteHostName } from "@/stores/remote_name";
import { GridLayout, GridItem } from 'vue3-grid-layout-next';
const layout = ref([])
const resize_switch = ref(false);
const my_components = {
  '0': IoPanel,
  '1': StateMachine,
  '2': LogExplore,
  '3': XlrdShow,
  '4': LiveCamera,
}
// 保存布局到本地存储
const saveLayout = (newLayout) => {
  localStorage.setItem('dashboard-layout', JSON.stringify(newLayout))
}

function reset_layout() {
  layout.value = [
    { x: 0, y: 0, w: 4, h: 6, i: '0' },
    { x: 4, y: 0, w: 6, h: 6, i: '1' },
    { x: 0, y: 6, w: 3, h:15, i: '2' },
    { x: 10, y: 0, w: 2, h: 6, i: '3' },
    { x: 3, y: 6, w: 9, h:15, i: '4' },
  ]
  saveLayout(layout.value);
}

// 恢复布局
const loadLayout = () => {
  const saved = localStorage.getItem('dashboard-layout')
  if (saved) layout.value = JSON.parse(saved)
}
onMounted(() => {
  loadLayout()
  if (layout.value.length === 0) {
    reset_layout();
  }
})
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
