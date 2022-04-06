import net from 'net';

class Piper
{
    public static readonly localConnectTimeout = 3000;

    remoteSocket: net.Socket;
    localSocket: net.Socket;
    anyCloseWasCalled: boolean;
    localPort: number;
    connectTimeout: any;

    constructor(brokerPort: number, localPort: number, brokerHost: string, onAnyClose: () => void) 
    {
        this.anyCloseWasCalled = false;
        this.localPort = localPort;

        const anyClose = () => {
            if (!this.anyCloseWasCalled)
            {
                this.anyCloseWasCalled = true;
                onAnyClose();
            }
        }
        this.connectTimeout = setTimeout(() => {
            console.log(`Connect to local service under port ${this.localPort} failed`);
            this.free();
        }, Piper.localConnectTimeout);

        this.remoteSocket = new net.Socket({});
        this.remoteSocket.connect({
            host: brokerHost,
            port: brokerPort
        });
        this.remoteSocket.on('error', (error) => {
            console.log(`Error with remote socket for service with local port${this.localPort}`, error);
        })
        this.remoteSocket.on('close', () => {
            console.log('Remote closed.');
            this.localSocket.destroy();
            this.localSocket.unref();
            anyClose();
        })
        this.localSocket = new net.Socket({});
        this.localSocket.connect({
            host: 'localhost',
            port: localPort
        });
        this.localSocket.on('error', (error) => {
            console.log(`Error with local socket for service with local port${this.localPort}`, error);
        })
        this.localSocket.on('close', () => {
            console.log('Local closed.');
            this.remoteSocket.destroy();
            this.remoteSocket.unref();
            anyClose();
        })
        this.makePipes();
    }

    makePipes = () => {
        this.remoteSocket.on('connect', () => {
            clearTimeout(this.connectTimeout);
            this.remoteSocket.pipe(this.localSocket);
        })
        this.localSocket.on('connect', () => {
            clearTimeout(this.connectTimeout);
            this.localSocket.pipe(this.remoteSocket);
        })
    }

    free = () => {
        this.remoteSocket.end();
        this.remoteSocket.destroy();
        this.remoteSocket.unref();
        this.localSocket.end();
        this.localSocket.destroy();
        this.localSocket.unref();
    }
}

export default Piper;