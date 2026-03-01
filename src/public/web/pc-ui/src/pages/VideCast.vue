<template>
    <div class="container">
        <div class="iframe-section">
            <iframe :key="iframeKey" :src="video_cast_url + '/'"></iframe>
        </div>
        <div class="text-section">
            <div class="text-block1">{{ text1 }}</div>
            <div class="text-block2">{{ text2 }}</div>
            <div class="text-block3">{{ text3 }}</div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue';
import { DataSyncClient } from "../ws_sync_client";
const client = new DataSyncClient('/ws/');
const text1 = ref("");
const text2 = ref("欢迎");
const text3 = ref("");
const iframeKey = ref(0);
const video_cast_url = ref('');
let refreshTimer = null;

onMounted(() => {
    refreshTimer = setInterval(() => {
        window.location.reload();
    }, 20000);
});

onBeforeUnmount(() => {
    if (refreshTimer !== null) {
        clearInterval(refreshTimer);
        refreshTimer = null;
    }
});

client.watchData((key, value) => {
    if (key === 'video_cast') {
        let orig_url = video_cast_url.value;
        video_cast_url.value = value.url;
        text2.value = value.prompt;
        text1.value = value.plate;
        text3.value = value.weight;
        if (video_cast_url.value != orig_url)
        {
        iframeKey.value += 1;  // 强制刷新 iframe
        }
        if (video_cast_url.value === '') {
            video_cast_url.value = 'about:blank';
        }
    }
}, 'video_cast');
</script>

<style scoped>
.container {
    display: flex;
    flex-direction: column;
    width: 100vw;
    height: 100vh;
    background: #f7f8fa;
}

.iframe-section {
    flex: 4 1 80%;
    min-height: 0;
}

.iframe-section iframe {
    width: 100%;
    height: 100%;
    border: none;
    display: block;
}

.text-section {
    flex: 1 1 20%;
    display: flex;
    gap: 12px;
    padding: 12px;
    box-sizing: border-box;
}
.text-block1 {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    background: #ffffff;
    border: 1px solid #e0e0e0;
    border-radius: 8px;
    font-size: 60px;
    font-weight: 700;
    font-family: "SimHei", "黑体", sans-serif;
    color: #333333;
}
.text-block2 {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    background: #ffffff;
    border: 1px solid #e0e0e0;
    border-radius: 8px;
    font-size: 60px;
    font-weight: 700;
    font-family: "SimHei", "黑体", sans-serif;
    color: #333333;
}

.text-block3 {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    background: #ffffff;
    border: 1px solid #e0e0e0;
    border-radius: 8px;
    font-size: 100px;
    font-weight: 700;
    font-family: "SimHei", "黑体", sans-serif;
    color: #333333;
}
</style>