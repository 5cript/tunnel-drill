import dgram from 'dgram';
import compareAddressStrings from '../../util/ip';

class UdpSession
{
    forwardingSocket: dgram.Socket;
    socketType: dgram.SocketType;
    onAnyClose: () => void
    anyCloseWasCalled: boolean;
    remotePort: number;
    localPort: number;
    brokerHost: string;
    onBound: (port: number) => void;

    constructor({remotePort, localPort, brokerHost, token, socketType, onAnyClose, onBound}: 
        {
            remotePort: number, 
            localPort: number, 
            brokerHost: string, 
            token: string, 
            socketType: string, 
            onAnyClose: () => void, 
            onBound: (port: number) => void
        }
    ) 
    {
        this.remotePort = remotePort;
        this.localPort = localPort;
        this.brokerHost = brokerHost;
        this.onBound = onBound;

        this.onAnyClose = () => {
            if (!this.anyCloseWasCalled)
            {
                this.anyCloseWasCalled = true;
                onAnyClose();
            };
        }

        switch(socketType.toLowerCase())
        {
            case 'udp4': 
            {
                this.socketType = 'udp4';
                break;
            }
            case 'udp6':
            {
                this.socketType = 'udp6';
                break;   
            }
        }

        this.makeSocket();
    }

    getPort = () => {
        return this.forwardingSocket.address().port;
    }

    makeSocket = () => 
    {
        this.forwardingSocket = dgram.createSocket({
            type: this.socketType,
            reuseAddr: true,
        });

        this.forwardingSocket.on('error', (error) => {
            this.free();
            this.onAnyClose();
        })
        this.forwardingSocket.on('listening', () => {
            this.onBound(this.getPort());
            this.forwardingSocket.on('message', (msg, {address, family, port, size}) => {
                const index = address.lastIndexOf('%');
                let addr = '';
                let iface = '';
                if (index != -1) {
                    addr = address.substring(0, index);
                    iface = address.substring(index + 1, address.length - index - 1);
                } else {
                    addr = address;
                }
                // broker host sent this:
                if (compareAddressStrings(addr, this.brokerHost))
                    this.forwardingSocket.send(msg, this.localPort, "localhost");
                else
                    this.forwardingSocket.send(msg, this.remotePort, this.brokerHost);
            })      
        })
        this.forwardingSocket.bind();
    }

    free = () => {
        this.forwardingSocket.close();
        this.forwardingSocket.unref();
    }
}

export default UdpSession;