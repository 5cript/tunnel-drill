import { WebSocket } from 'ws';
import { ServiceInfo } from '../../shared/service';
import { AcceptorHandle, TcpAcceptorPool } from './acceptor_pool';
import net from 'net';
import { generateUuid } from './util/id';
import { NewTunnelMessage, NewTunnelFailedMessage } from '../../shared/control_messages/new_tunnel';
import compareAddressStrings from '../../util/ip';
import { ServicesMessage } from '../../shared/control_messages/services';
import SharedConstants from '../../shared/shared_constants';
import UdpRelayPool from './relay_pool';
import winston from 'winston';

class Session
{
    clientSocket: net.Socket;
    publisherSocket: net.Socket | undefined;
    initialData: Buffer;

    constructor(socket: net.Socket, initialData: Buffer)
    {
        this.clientSocket = socket;
        this.initialData = initialData;
        this.publisherSocket = undefined;
    }

    connect = (publisherSocket: net.Socket) => {
        this.publisherSocket = publisherSocket;

        this.publisherSocket.write(this.initialData, undefined, () => {
            this.publisherSocket.pipe(this.clientSocket);
            this.clientSocket.pipe(this.publisherSocket);
            this.publisherSocket.resume();
            this.clientSocket.resume();
        });
    }

    isHalfOpen = () => {
        return this.publisherSocket === undefined;
    }

    free = () => {
        // FIXME: Improve me:
        setTimeout(() => {
            this.clientSocket.destroy();
            this.clientSocket.unref();
            this.publisherSocket?.destroy();
            this.publisherSocket?.unref();
        }, 5000); // 5 seconds to end all IO
    }
};

class PublishedService
{
    name?: string;
    hiddenPort: number;
    publicPort: number;
    acceptorHandle: AcceptorHandle;
    sessions: {[tunnelId: string]: Session};

    constructor({hiddenPort, publicPort, name, acceptorHandle}: 
        {hiddenPort: number, publicPort: number, name?: string, acceptorHandle: AcceptorHandle})
    {
        this.hiddenPort = hiddenPort;
        this.publicPort = publicPort;
        this.name = name;
        this.acceptorHandle = acceptorHandle;
        this.sessions = {};
    }

    addSocketToHalfOpenSession = (tunnelId: string, publisherSocket: net.Socket) => {
        if (this.sessions[tunnelId].isHalfOpen())
        {
            publisherSocket.on('error', (error) => {
                winston.error(`Error in publisher socket for session id ${tunnelId} with error ${error.message}`);
            })
            publisherSocket.on('close', () => {
                this.freeSession(tunnelId);
            })
            this.sessions[tunnelId].connect(publisherSocket);
        }
    }

    freeSession = (tunnelId: string) => {
        if (!this.sessions[tunnelId])
            return;

        this.sessions[tunnelId].free();
        delete this.sessions[tunnelId];
    }

    freeAll = () => {
        for (const id in this.sessions)
            this.freeSession(id);
    }
};

class Publisher
{
    controlSocket: WebSocket;
    services: {[key: string]: PublishedService};
    tcpServers: TcpAcceptorPool;
    updRelays: UdpRelayPool;
    publisherAddress: string;
    messageMap: {[messageName: string]: (...args: any) => void}

    constructor(socket: WebSocket, publisherAddress: string, limit: number)
    {
        this.controlSocket = socket;
        this.services = {};
        this.tcpServers = new TcpAcceptorPool(limit);
        this.publisherAddress = publisherAddress;
        this.messageMap = {};

        socket.on('message', (data, isBinary) => {
            if (!isBinary) {
                const msg = JSON.parse(data.toString());
                this.messageMap[msg.type](msg);
            }
        })

        // register understood messages:
        this.messageMap["NewTunnelFailed"] = (messageObject: NewTunnelFailedMessage) => {
            this.onSessionFailed(messageObject);
        }
        this.messageMap["Services"] = (messageObject: ServicesMessage) => {
            this.setServices(messageObject.services)
        }
    }

    freeSessionForService(serviceId: string, tunnelId: string)
    {
        if (this.services[serviceId])
        {
            this.services[serviceId].freeSession(tunnelId);
            delete this.services[serviceId].sessions[tunnelId];
        }
    }

    onSessionFailed = ({hiddenPort, publicPort, serviceId, tunnelId, socketType, reason}: NewTunnelFailedMessage) => {
        if (!this.services[serviceId])
        {
            winston.error(`onSessionFailed for unknown service id ${serviceId}.`);
            return;
        }
        winston.error(`session ${tunnelId} failed for service ${serviceId}(${hiddenPort}, ${socketType}) for reason ${reason}`);
        this.freeSessionForService(serviceId, tunnelId);
    }

    clearServices = () => {
        this.tcpServers.freeAll();
        for (const serviceId in this.services)
            this.services[serviceId].freeAll();
        this.services = {};
    }

    setServices = (serviceInfos: Array<ServiceInfo>) =>
    {
        this.clearServices();

        serviceInfos.forEach(service => {
            const serviceId = generateUuid();
            const acceptorHandle = this.tcpServers.makeAcceptor(service.publicPort, (socket: net.Socket) => {
                this.onConnect(serviceId, socket);
            })
            if (!acceptorHandle)
                return;
            this.services[serviceId] = new PublishedService({
                publicPort: service.publicPort, 
                hiddenPort: service.hiddenPort,
                name: service.name ? service.name : serviceId, 
                acceptorHandle
            });
        })
    }

    private onConnect = (serviceId: string, socket: net.Socket) => {
        if (!this.services[serviceId])
        {
            socket.end();
            winston.error(`Connection opened for unknown service ${serviceId}`);
            return;
        }

        const doInitialConnect = (socket: net.Socket, initialData: Buffer) => {
            winston.info(`New session for service ${this.services[serviceId].name}`);
            const tunnelId = generateUuid();
            socket.on('error', (error) => {
                winston.error(`Error in client socket for session id ${tunnelId}: ${error.message}.`);
            })
            this.services[serviceId].sessions[tunnelId] = new Session(socket, initialData);
            this.controlSocket.send(JSON.stringify({
                type: 'NewTunnel',
                serviceId: serviceId,
                tunnelId: tunnelId,
                hiddenPort: this.services[serviceId].hiddenPort,
                publicPort: this.services[serviceId].publicPort 
            } as NewTunnelMessage));
        };
        const doPublisherConnect = (socket: net.Socket, {tunnelId, serviceId}: {tunnelId: string, serviceId: string}) => {
            socket.on('error', (error) => {
                winston.error(`Error in client socket for session id ${tunnelId}: ${error.message}.`);
            })
            this.services[serviceId].addSocketToHalfOpenSession(tunnelId, socket);
            return;
        }

        const sameAddress = compareAddressStrings(socket.remoteAddress, this.publisherAddress);
        socket.once("data", (peekBuffer: Buffer) => {
            socket.pause();
            try {
                const token = JSON.parse(peekBuffer.toString('utf-8'));
                if (token && token.tunnelId && token.serviceId && sameAddress) 
                {
                    doPublisherConnect(
                        socket, 
                        {
                            tunnelId: token.tunnelId,
                            serviceId: token.serviceId
                        }
                    );
                    return;
                }
            }
            catch(e)
            {
            }
        
            let initialBuf = Buffer.alloc(peekBuffer.length);
            peekBuffer.copy(initialBuf);
            doInitialConnect(socket, initialBuf);
        });
    }

    close = () => {
        this.clearServices();
        this.controlSocket.close();
    }
}

export default Publisher;