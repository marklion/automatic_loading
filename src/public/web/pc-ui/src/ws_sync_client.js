export class DataSyncClient {
    constructor(url, options = {}) {
        this.url = url;
        this.ws = null;
        this._dataStore = new Map();
        this._listeners = new Map();
        this._pendingMessages = [];

        this._manualClose = false;
        this._reconnectTimer = null;
        this._reconnectAttempts = 0;

        this._reconnectOptions = {
            initialDelay: options.initialDelay || 1000,
            maxDelay: options.maxDelay || 30000,
            backoffFactor: options.backoffFactor || 2,
            jitter: options.jitter || 0.2
        };

        this._connect();
    }

    _connect() {
        this.ws = new WebSocket(this.url);

        this.ws.onopen = () => {
            this._reconnectAttempts = 0;
            this._flushPendingMessages();
        };

        this.ws.onmessage = (message) => {
            const { type, key, value, data } = JSON.parse(message.data);
            if (type === 'init') {
                Object.entries(data).forEach(([k, v]) => this._dataStore.set(k, v));
            } else if (type === 'update') {
                this._dataStore.set(key, value);
                this._triggerListeners(key, value);
            }
        };

        this.ws.onclose = () => {
            this._scheduleReconnect();
        };

        this.ws.onerror = () => {
            if (this.ws && this.ws.readyState === WebSocket.OPEN) {
                this.ws.close();
            }
        };
    }

    _scheduleReconnect() {
        if (this._manualClose || this._reconnectTimer) {
            return;
        }

        const { initialDelay, maxDelay, backoffFactor, jitter } = this._reconnectOptions;
        const baseDelay = Math.min(
            maxDelay,
            initialDelay * Math.pow(backoffFactor, this._reconnectAttempts)
        );
        const jitterRange = baseDelay * jitter;
        const delay = Math.max(0, baseDelay + (Math.random() * 2 - 1) * jitterRange);

        this._reconnectTimer = setTimeout(() => {
            this._reconnectTimer = null;
            this._reconnectAttempts += 1;
            this._connect();
        }, delay);
    }

    _send(payload) {
        const message = JSON.stringify(payload);
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(message);
            return;
        }

        this._pendingMessages.push(message);
    }

    _flushPendingMessages() {
        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
            return;
        }

        while (this._pendingMessages.length > 0) {
            this.ws.send(this._pendingMessages.shift());
        }
    }

    _triggerListeners(key, value) {
        const globalListeners = this._listeners.get('global') || [];
        const keyListeners = this._listeners.get(key) || [];
        [...globalListeners, ...keyListeners].forEach(cb => cb(key, value));
    }

    setData(key, value) {
        this._dataStore.set(key, value);
        this._send({ type: 'set', key, value });
    }

    watchData(callback, specificKey) {
        const key = specificKey || 'global';
        this._listeners.set(key, [...(this._listeners.get(key) || []), callback]);
    }

    close() {
        this._manualClose = true;
        if (this._reconnectTimer) {
            clearTimeout(this._reconnectTimer);
            this._reconnectTimer = null;
        }

        if (this.ws && this.ws.readyState <= WebSocket.OPEN) {
            this.ws.close();
        }
    }
}