<template>
    <div class="container">
        <div class="iframe-section">
            <iframe :key="iframeKey" :src="video_cast_url + '/'"></iframe>
        </div>
        <div class="text-section">
            <div class="text-block">{{ text1 }}</div>
            <div class="text-block">{{ text2 }}</div>
            <div class="text-block">{{ text3 }}</div>
        </div>
    </div>
</template>

<script setup>
import { ref } from 'vue';
import { DataSyncClient } from "../ws_sync_client";
const client = new DataSyncClient('/ws/');
const text1 = ref("文本一");
const text2 = ref("文本二");
const text3 = ref("文本三");
const iframeKey = ref(0);
const video_cast_url = ref('');
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

.text-block {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    background: #ffffff;
    border: 1px solid #e0e0e0;
    border-radius: 8px;
    font-size: 42px;
    color: #333333;
}
</style>