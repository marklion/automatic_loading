<template>
<div>
    <el-input v-model="remote_hostname" placeholder="Please input">
    </el-input>
    <IoPanel>
    </IoPanel>
</div>
</template>

<script setup>
import IoPanel from '../../../../../modbus_io/web/io_panel.vue'
import {
    ref,
    computed
} from "vue";
import {useRemoteHostName} from "@/stores/remote_name";
const hostname_store = useRemoteHostName();
const remote_hostname = computed({
    get: () => hostname_store.remoteName,
    set: (value) => hostname_store.setRemoteName(value)
});
</script>

<style>

</style>
