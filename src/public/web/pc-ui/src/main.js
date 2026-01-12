import { createApp } from 'vue'
import App from './App.vue'
import router from './router'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import { createPinia } from 'pinia'
import { useRemoteHostName } from "@/stores/remote_name";
import axios from "axios";
import * as ElementPlusIconsVue from '@element-plus/icons-vue'

const app = createApp(App)
const pinia = createPinia()

for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
    app.component(key, component)
}

app.use(pinia)
app.use(ElementPlus)
app.use(router)
app.mount('#app')
const hostname_store = useRemoteHostName();
app.config.globalProperties.$call_remote_cli = async function (cmd) {
    const remote_name = hostname_store.remoteName;
    let encoded_cmd = encodeURIComponent(cmd);
    let ret = "";
    try {
        let resp = await axios.get(`${remote_name}/api/cli?cmd=${encoded_cmd}`);
        ret = resp.data;
    } catch (error) {
        console.log(error);
    }
    return ret;
}
