import net from 'net';
import SharedConstants from '../../shared/shared_constants';
import winston from 'winston';

class TcpSession
{
    public static readonly localConnectTimeout = 3000;

    remoteSocket: net.Socket;
    localSocket: net.Socket;
    anyCloseWasCalled: boolean;
    localPort: number;
    remotePort: number;
    onAnyClose: () => void;

    constructor(brokerPort: number, localPort: number, brokerHost: string, token: string, onAnyClose: () => void) 
    {
        this.anyCloseWasCalled = false;
        this.localPort = localPort;
        this.remotePort = brokerPort;

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
            winston.error(`Error with local socket for service with local port ${this.localPort}: ${error.message}`);
        })
        this.localSocket.on('close', () => {
            // FIXME: improveMe
            setTimeout(() => {
                this.remoteSocket.destroy();
                this.remoteSocket.unref();
                this.onAnyClose();
            }, 5000); // 5 seconds to end all IO
        })
        this.localSocket.on('connect', () => {
            winston.info(`Both ends are open, starting to pipe between broker at ${this.remotePort} and service at ${this.localPort}`);
            this.localSocket.write(initialData, undefined, () => {
                this.remoteSocket.pipe(this.localSocket);
                this.localSocket.pipe(this.remoteSocket);
                this.remoteSocket.resume();
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
            winston.error(`Error with remote socket for service with local port ${this.localPort}: ${error.message}`);
        })
        this.remoteSocket.on('close', () => {
            // FIXME: improveMe
            setTimeout(() => {
                this.localSocket.destroy();
                this.localSocket.unref();
                this.onAnyClose();
            }, 5000) // 5 seconds to end all IO
        })

        this.remoteSocket.on('connect', () => {
            this.remoteSocket.write(token, 'utf-8', () => {
            });
            this.remoteSocket.once('data', (buffer) => {
                this.remoteSocket.pause();
                let initialBuf = Buffer.alloc(buffer.length);
                buffer.copy(initialBuf);
                this.connectLocalSocket(initialBuf);
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

export default TcpSession;