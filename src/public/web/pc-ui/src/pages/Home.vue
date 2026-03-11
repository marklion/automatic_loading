<template>
  <div>
    <el-row>
      <el-col :span="8">
        <el-menu default-active="0" mode="horizontal" :ellipsis="false" @select="handleSelect">
          <el-menu-item index="0">
            操作中心
          </el-menu-item>
          <el-menu-item index="1">配置工具</el-menu-item>
          <el-menu-item index="2">更新</el-menu-item>
        </el-menu>
      </el-col>
      <el-col :span="8" v-if="current_nav_index == '0'">
        <el-switch v-model="resize_switch" active-text="允许调整大小" inactive-text="禁止调整大小"></el-switch>
        <el-button @click="reset_layout">重置布局</el-button>
      </el-col>
    </el-row>
    <grid-layout v-if="current_nav_index == '0'" :layout="layout" :col-num="12" :row-height="30"
      :is-draggable="resize_switch" :is-resizable="resize_switch" :auto-size="false" @layout-updated="saveLayout">
      <grid-item v-for="item in layout" :key="item.i" :x="item.x" :y="item.y" :w="item.w" :h="item.h" :i="item.i">
        <!-- 你的组件内容 -->
        <div style="width: 100%; height: 100%; padding:5px;">
          <component :is="my_components[item.i]"></component>
        </div>
      </grid-item>
    </grid-layout>
    <div v-else-if="current_nav_index == '1'">
      <iframe src="/wetty" style="width: 100vw; height: 80vh; border: none"></iframe>
    </div>
    <div v-else-if="current_nav_index == '2'">
      <el-upload action="/api/upload_firmware" :limit="1" :on-success="confirm_update">
        <el-button type="primary">上传</el-button>
      </el-upload>
    </div>
  </div>
</template>

<script setup>
import IoPanel from "../../../../../modbus_io/web/io_panel.vue";
import StateMachine from "../../../../../state_machine/web/state_machine.vue";
import LogExplore from "../../../../../log/web/log_explore.vue";
import XlrdShow from "../../../../../xlrd/web/xlrd_show.vue";
import LiveCamera from "../../../../../live_camera/web/live_camera.vue";
import DropSystem from "../../../../../drop_system/web/drop_system.vue";
//import PcdShow from "@/components/PcdShow.vue";
import Scale from "../../../../../scale/web/scale.vue";
import { ref, computed, onMounted } from "vue";
import { useRemoteHostName } from "@/stores/remote_name";
import { GridLayout, GridItem } from 'vue3-grid-layout-next';
import { ElMessageBox } from 'element-plus'
import axios from "axios";
const layout = ref([])
const current_nav_index = ref('0');
const resize_switch = ref(false);
const my_components = {
  '0': IoPanel,
  '1': StateMachine,
  '2': LogExplore,
  '3': XlrdShow,
  '4': LiveCamera,
  '5': Scale,
  '6': DropSystem,
}
// 保存布局到本地存储
const saveLayout = (newLayout) => {
  localStorage.setItem('dashboard-layout', JSON.stringify(newLayout))
}

function reset_layout() {
  layout.value =
    [
      {
        "x": 0,
        "y": 20,
        "w": 2,
        "h": 3,
        "i": "6",

      },
      {
        "x": 0,
        "y": 0,
        "w": 2,
        "h": 20,
        "i": "0",

      },
      {
        "x": 2,
        "y": 0,
        "w": 7,
        "h": 7,
        "i": "1",

      },
      {
        "x": 9,
        "y": 0,
        "w": 3,
        "h": 5,
        "i": "2",

      },
      {
        "x": 9,
        "y": 5,
        "w": 2,
        "h": 3,
        "i": "3",

      },
      {
        "x": 2,
        "y": 8,
        "w": 10,
        "h": 14,
        "i": "4",

      },
      {
        "x": 11,
        "y": 5,
        "w": 1,
        "h": 2,
        "i": "5",

      }
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
function handleSelect(key, keyPath) {
  current_nav_index.value = key;
}
async function confirm_update(response, file, fileList) {
  if (response.status == "success") {
    ElMessageBox.confirm('固件上传成功，是否立即更新系统？', '确认', {
      confirmButtonText: '更新',
      cancelButtonText: '取消',
      type: 'warning',
    }).then(async () => {
      await axios.post("/api/update_system", {});
      ElMessageBox.alert('系统将重启以应用更新。', '提示', {
        confirmButtonText: '确定',
        callback: () => {
          window.location.reload();
        }
      });
    }).catch(() => {
    });

  } else {
    alert("固件上传失败：" + response.message);
  }
}
</script>

<style></style>
