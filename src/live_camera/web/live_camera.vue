<template>
    <div style="height: 100%;">
        <div style="display: grid; grid-template-columns: repeat(1, 1fr); gap: 20px;height: 100%">
            <div v-for="(cam, index) in video_urls" :key="index">
                <iframe :src="cam.url" frameborder="0" allowfullscreen style="width: 100%; height: 100%; border: 1px solid #ccc; border-radius: 4px;"></iframe>
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

<style lang="scss" scoped></style>