import net from 'net';
import SharedConstants from '../../shared/shared_constants';
import winston from 'winston';

class TcpSession {
    public static readonly localConnectTimeout = 3000;

    remoteSocket: net.Socket;
    localSocket: net.Socket;
    anyCloseWasCalled: boolean;
    hiddenPort: number;
    hiddenHost: string;
    remotePort: number;
    total: number;
    onAnyClose: () => void;

    constructor(publicPort: number, hiddenPort: number, brokerHost: string, token: string, onAnyClose: () => void, hiddenHost?: string) {
        this.anyCloseWasCalled = false;
        this.hiddenPort = hiddenPort;
        this.remotePort = publicPort;
        this.hiddenHost = hiddenHost ? hiddenHost : "localhost";
        this.total = 0;

        this.onAnyClose = () => {
            console.log(this.total);
            if (!this.anyCloseWasCalled) {
                this.anyCloseWasCalled = true;
                onAnyClose();
            }
        }

        this.connectRemoteSocket(brokerHost, publicPort, token);
    }

    private connectLocalSocket = () => {
        this.localSocket = new net.Socket({});
        this.localSocket.connect({
            host: this.hiddenHost,
            port: this.hiddenPort
        });
        this.localSocket.pause();
        this.localSocket.on('error', (error) => {
            winston.error(`Error with local socket for service with local port ${this.hiddenPort}: ${error.message}`);
        })
        this.localSocket.on('close', () => {
            winston.info('LocalClose');
            // FIXME: improveMe
            setTimeout(() => {
                this.remoteSocket?.end();
                this.remoteSocket?.destroy();
                this.remoteSocket?.unref();
                this.onAnyClose();
            }, 8000) // 1 sec to write stuff.
        })
        this.localSocket.on('connect', () => {
            winston.info(`Both ends are open, starting to pipe between broker at ${this.remotePort} and service at ${this.hiddenPort}`);
            // this.localSocket.on('data', (data) => {
            //     this.remoteSocket.write(data);
            // })
            // this.remoteSocket.on('data', (data) => {
            //     this.localSocket.write(data);
            // })
            this.localSocket.pipe(this.remoteSocket);
            this.remoteSocket.pipe(this.localSocket);
            this.localSocket.resume();
            this.remoteSocket.resume();
        })
    }

    private connectRemoteSocket = (brokerHost: string, publicPort: number, token: string) => {
        this.remoteSocket = new net.Socket({});
        this.remoteSocket.connect({
            host: brokerHost,
            port: publicPort
        });
        this.remoteSocket.pause();
        this.remoteSocket.on('error', (error) => {
            winston.error(`Error with remote socket for service with local port ${this.hiddenPort}: ${error.message}`);
        })
        this.remoteSocket.on('close', () => {
            winston.info('RemoteClose');
            setTimeout(() => {
                this.localSocket?.end();
                this.localSocket?.destroy();
                this.localSocket?.unref();
                this.onAnyClose();
            }, 8000)
        });
        this.remoteSocket.on('connect', () => {
            this.remoteSocket.write(token, 'utf-8', () => {
                this.connectLocalSocket();
            });
        });
    }

    free = () => {
        this.remoteSocket?.end();
        this.remoteSocket?.destroy();
        this.remoteSocket?.unref();
        this.localSocket?.end();
        this.localSocket?.destroy();
        this.localSocket?.unref();
    }
}

export default TcpSession;