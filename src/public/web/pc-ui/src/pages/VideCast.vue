<template>
    <div class="container">
        <div class="iframe-section">
            <iframe :key="iframeKey" :src="video_cast_url + '/'"></iframe>
        </div>
        <div class="text-section">
            <div class="text-block1">{{ text1 }}</div>
            <div :class="['text-block2', { 'text-block2-stop': text2.includes('停车') }]">{{ text2 }}</div>
            <div class="text-block3">{{ text3 }}</div>
        </div>
    </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue';
import { DataSyncClient } from "../ws_sync_client";
import Speech from 'speak-tts';
const client = new DataSyncClient('/ws/');
const text1 = ref("");
const text2 = ref("欢迎");
const text3 = ref("");
const iframeKey = ref(0);
const video_cast_url = ref('');
let refreshTimer = null;
let ann_timer = null;

let ann_content = '';
let ann_gap = -1;
let ann_should_refresh = false;
let ann_gap_counter = 0;

onMounted(async () => {
    refreshTimer = setInterval(() => {
        window.location.reload();
    }, 60 * 1000 * 40);
    ann_timer = setInterval(() => {
        if (ann_should_refresh) {
            do_ann(ann_content);
            ann_should_refresh = false;
        }
        ann_gap_counter += 0.2;
        if (ann_gap > 0 && ann_gap_counter >= ann_gap) {
            ann_should_refresh = true;
            ann_gap_counter = 0;
        }
    }, 200);
    await speech.init({
        volume: 1, // 音量
        lang: 'zh-CN', // 语言，设置为中文
        rate: 1.2, // 语速
        pitch: 1, // 音调,
        voice:"Google 普通话（中国大陆）",
    });
});

onBeforeUnmount(() => {
    if (refreshTimer !== null) {
        clearInterval(refreshTimer);
        refreshTimer = null;
    }
    if (ann_timer !== null) {
        clearInterval(ann_timer);
        ann_timer = null;
    }
});
const speech = new Speech();


// 播报方法
const speak = async (text) => {
    if (speech.hasBrowserSupport()) {
        await speech.cancel();
        await speech.speak({
            text: text,
        });
    }
};
async function do_ann(content) {
    if (content && content.trim() !== '') {
        await speak(content);
    }
}

client.watchData((key, value) => {
    if (key === 'video_cast') {
        let orig_url = video_cast_url.value;
        video_cast_url.value = value.url;
        text2.value = value.prompt;
        text1.value = value.plate;
        text3.value = value.weight;
        if (video_cast_url.value != orig_url) {
            iframeKey.value += 1;  // 强制刷新 iframe
        }
        if (video_cast_url.value === '') {
            video_cast_url.value = 'about:blank';
        }
        if (ann_content != value.ann.content || ann_gap != value.ann.gap) {
            ann_content = value.ann.content;
            ann_gap = value.ann.gap;
            ann_should_refresh = true;
            ann_gap_counter = 0;
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
    flex: 4 1 20%;
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
    border: 2px solid #093fe2;
    border-radius: 8px;
    font-size: 90px;
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
    border: 2px solid #093fe2;
    border-radius: 8px;
    font-size: 90px;
    letter-spacing: 0.08em;
    font-weight: 700;
    font-family: "SimHei", "黑体", sans-serif;
    color: #063d96;
}

.text-block2-stop {
    background: #ff0000;
}

.text-block3 {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    background: #ffffff;
    border: 2px solid #093fe2;
    border-radius: 8px;
    font-size: 100px;
    font-weight: 700;
    font-family: "SimHei", "黑体", sans-serif;
    color: #333333;
}
</style>