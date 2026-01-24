import { defineStore } from "pinia";
export const useSmEvent = defineStore("smEvent", {
    state: () => ({
        'to': '',
        'from': '',
        'event': '',
    }),
    actions: {
        setTo(data) {
            this.to = data;
        },
        setFrom(data) {
            this.from = data;
        },
        setEvent(data) {
            this.event = data;
        },
    },
});