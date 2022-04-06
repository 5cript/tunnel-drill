import net from 'net';
import SharedConstants from './shared_constants';

class Piper
{
    public static readonly localConnectTimeout = 3000;

    remoteSocket: net.Socket;
    localSocket: net.Socket;
    anyCloseWasCalled: boolean;
    localPort: number;
    onAnyClose: () => void;

    constructor(brokerPort: number, localPort: number, brokerHost: string, token: string, onAnyClose: () => void) 
    {
        this.anyCloseWasCalled = false;
        this.localPort = localPort;

        this.onAnyClose = () => {
            if (!this.anyCloseWasCalled)
            {
                this.anyCloseWasCalled = true;
                onAnyClose();
            }
        }

        this.connectRemoteSocket(brokerHost, brokerPort, token);
    }

    private connectLocalSocket = (initialData: Buffer) => {
        this.localSocket = new net.Socket({});
        this.localSocket.connect({
            host: 'localhost',
            port: this.localPort
        });
        this.localSocket.on('error', (error) => {
            console.log(`Error with local socket for service with local port${this.localPort}`, error);
        })
        this.localSocket.on('close', () => {
            console.log('Local closed.');
            this.remoteSocket.destroy();
            this.remoteSocket.unref();
            this.onAnyClose();
        })
        this.localSocket.on('connect', () => {
            console.log('Both ends are open, starting to pipe.');
            this.localSocket.write(initialData, undefined, () => {
                this.remoteSocket.pipe(this.localSocket);
                this.localSocket.pipe(this.remoteSocket);
            })
        })
    }

    private connectRemoteSocket = (brokerHost: string, brokerPort: number, token: string) => {
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
            this.onAnyClose();
        })

        this.remoteSocket.on('connect', () => {
            this.remoteSocket.write(token, 'utf-8', () => {
            });
            this.remoteSocket.once('data', (buffer) => {
                this.connectLocalSocket(buffer);
            })
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