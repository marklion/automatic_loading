import { defineStore } from "pinia";
export const useRemoteHostName = defineStore("remoteName", {
    state: () => ({
        remoteName: "",
    }),
    actions: {
        setRemoteName(name) {
            this.remoteName = name;
        },
    },
});