import { defineStore } from "pinia";
export const useStatusInfo = defineStore("statusInfo", {
    state: () => ({
        'modbus_io': [],
        'sm': {},
        'xlrd0': {},
        'xlrd1': {},
    }),
    actions: {
        setModbusIO(data) {
            this.modbus_io = data;
        },
        setStateMachine(data) {
            this.sm = data;
        },
        setXLRD0(data) {
            this.xlrd0 = data;
        },
        setXLRD1(data) {
            this.xlrd1 = data;
        },
    },
});