<template>
  <div class="industrial-panel camera-page">
    <div class="toolbar">
      <div>
        <div class="panel-title">实时视频监控</div>
        <div class="panel-subtitle">共 {{ video_urls.length }} 路摄像头</div>
      </div>
      <div class="header-actions">
        <el-button class="industrial-btn" @click="fetch_camera_urls">刷新</el-button>
      </div>
    </div>

    <div v-if="video_urls.length === 0" class="empty-tip">暂无摄像头信号</div>
    <div v-else class="camera-grid">
      <div v-for="(cam, index) in video_urls" :key="index" class="camera-card">
        <div class="camera-title">{{ cam.name }}</div>
        <iframe :src="cam.url" frameborder="0" allowfullscreen class="camera-frame"></iframe>
      </div>
    </div>
  </div>
</template>

<script setup>
import { onMounted, ref, getCurrentInstance } from "vue";
const instance = getCurrentInstance();
const video_urls = ref([]);
async function fetch_camera_urls() {
    let resp = await instance.appContext.config.globalProperties.$call_remote_cli(
        "live_camera show_cameras"
    );
    video_urls.value = resp.map(item => { return { url: '/live/' + item.name + "/", name: item.name } });
}
onMounted(() => {
    fetch_camera_urls();
});
</script>

<style scoped>
@import "../../public/web/web_common/industrial_theme.css";

.camera-page {
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

.camera-grid {
  flex: 1;
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 10px;
  min-height: 0;
}

.camera-card {
  border: 1px solid rgba(95, 169, 231, 0.35);
  background: rgba(8, 28, 49, 0.38);
  border-radius: 8px;
  padding: 8px;
  display: flex;
  flex-direction: column;
  gap: 6px;
  min-height: 0;
}

.camera-title {
  font-size: 13px;
  color: #bfe3ff;
  font-weight: 700;
}

.camera-frame {
  flex: 1;
  width: 100%;
  border: 1px solid rgba(95, 169, 231, 0.3);
  border-radius: 4px;
  min-height: 0;
}

.empty-tip {
  color: #8ab5d6;
  font-size: 12px;
}
</style>