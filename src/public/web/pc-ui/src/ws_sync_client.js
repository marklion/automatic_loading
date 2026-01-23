export class DataSyncClient {
    constructor(url) {
        this.ws = new WebSocket(url);
        this._dataStore = new Map();
        this._listeners = new Map();

        this.ws.onmessage = (message) => {
            const { type, key, value, data } = JSON.parse(message.data);
            if (type === 'init') {
                Object.entries(data).forEach(([k, v]) => this._dataStore.set(k, v));
            } else if (type === 'update') {
                this._dataStore.set(key, value);
                this._triggerListeners(key, value);
            }
        };
    }

    _triggerListeners(key, value) {
        const globalListeners = this._listeners.get('global') || [];
        const keyListeners = this._listeners.get(key) || [];
        [...globalListeners, ...keyListeners].forEach(cb => cb(key, value));
    }

    setData(key, value) {
        this._dataStore.set(key, value);
        this.ws.send(JSON.stringify({ type: 'set', key, value }));
    }

    watchData(callback, specificKey) {
        const key = specificKey || 'global';
        this._listeners.set(key, [...(this._listeners.get(key) || []), callback]);
    }
}