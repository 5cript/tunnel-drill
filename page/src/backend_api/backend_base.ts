class BackendBase
{
    webSocket: WebSocket | undefined = undefined;

    private webSocketUrl = () => {
        const loc = window.location;
        let url = '';
        if (loc.protocol === "https:") {
            url = "wss:";
        } else {
            url = "ws:";
        }
        url += "//" + loc.host;
        url += "/api/ws/frontend";
        console.log(url)
        return url;
    }

    constructor() {
        this.connect();
    }    

    private connect = () => {
        try {
            this.webSocket = new WebSocket(this.webSocketUrl());
        } catch (e) {
        }
        this.setupWebsocket();
    }

    private setupWebsocket = () => {
        this.webSocket?.addEventListener('open', () => {
            this.webSocket?.send('Hi');
        });
        this.webSocket?.addEventListener('message', (message) => {
            console.log("ws msg:", message);
        });
        this.webSocket?.addEventListener('error', (error) => {
            console.log('Websocket error', error);
        });
        this.webSocket?.addEventListener('close', () => {
            // TODO: Display in UI.
            console.log('Socket closed, attempting reconnect in 5 seconds.');
            setTimeout(() => {
                this.connect()
            }, 5000)
        });
    }
}

export default BackendBase;