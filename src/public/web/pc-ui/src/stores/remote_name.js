import { defineStore } from "pinia";
export const useRemoteHostName = defineStore("remoteName", {
    state: () => ({
        remoteName: "127.0.0.1",
    }),
    actions: {
        setRemoteName(name) {
            this.remoteName = name;
        },
    },
});