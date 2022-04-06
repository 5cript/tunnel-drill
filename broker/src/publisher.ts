import { WebSocket } from 'ws';
import { ServiceInfo } from '../../shared/service';
import { AcceptorHandle, TcpAcceptorPool } from './acceptor_pool';
import net from 'net';
import { generateUuid } from './util/id';
import { NewSessionMessage, NewSessionFailedMessage } from '../../shared/control_messages/new_session';
import compareAddressStrings from '../../util/ip';
import { ServicesMessage } from '../../shared/control_messages/services';

class Session
{
    clientSocket: net.Socket;
    publisherSocket: net.Socket | undefined;

    constructor(socket: net.Socket)
    {
        this.clientSocket = socket;
        this.publisherSocket = undefined;
    }

    connect = (publisherSocket: net.Socket) => {
        this.publisherSocket = publisherSocket;
        
        this.clientSocket.pipe(this.publisherSocket);
        this.publisherSocket.pipe(this.clientSocket);
    }

    isHalfOpen = () => {
        return this.publisherSocket === undefined;
    }

    free = () => {
        this.clientSocket.destroy();
        this.clientSocket.unref();
        this.publisherSocket?.destroy();
        this.publisherSocket?.unref();
    }
};

class PublishedService
{
    name?: string;
    localPort: number;
    remotePort: number;
    acceptorHandle: AcceptorHandle;
    sessions: {[sessionId: string]: Session};

    constructor({localPort, remotePort, name, acceptorHandle}: 
        {localPort: number, remotePort: number, name?: string, acceptorHandle: AcceptorHandle})
    {
        this.localPort = localPort;
        this.remotePort = remotePort;
        this.name = name;
        this.acceptorHandle = acceptorHandle;
        this.sessions = {};
    }

    addSocketToAnyHalfOpenSession = (publisherSocket: net.Socket) => {
        for (const sessionId in this.sessions) {
            if (this.sessions[sessionId].isHalfOpen())
            {
                publisherSocket.on('error', (error) => {
                    console.log('Error in publisher socket for session Id', sessionId, error);
                })
                publisherSocket.on('close', () => {
                    this.freeSession(sessionId);
                })
                this.sessions[sessionId].connect(publisherSocket);
                break;
            }
        }
    }

    freeSession = (sessionId: string) => {
        if (!this.sessions[sessionId])
            return;

        this.sessions[sessionId].free();
        delete this.sessions[sessionId];
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
                console.log('Message from publisher:', msg);
                this.messageMap[msg.type](msg);
            }
        })

        // register understood messages:
        this.messageMap["NewSessionFailed"] = (messageObject: NewSessionFailedMessage) => {
            this.onSessionFailed(messageObject);
        }
        this.messageMap["Services"] = (messageObject: ServicesMessage) => {
            this.setServices(messageObject.services)
        }
    }

    freeSessionForService(serviceId: string, sessionId: string)
    {
        if (this.services[serviceId])
        {
            this.services[serviceId].freeSession(sessionId);
            delete this.services[serviceId].sessions[sessionId];
        }
    }

    onSessionFailed = ({localPort, remotePort, serviceId, sessionId, reason}: NewSessionFailedMessage) => {
        if (!this.services[serviceId])
        {
            console.log('onSessionFailed for unknown service id', serviceId);
            return;
        }
        console.log(`session ${sessionId} failed for service ${serviceId}(${localPort}) for reason ${reason}`);
        this.freeSessionForService(serviceId, sessionId);
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
            const acceptorHandle = this.tcpServers.makeAcceptor(service.remotePort, (socket: net.Socket) => {
                this.onConnect(serviceId, socket);
            })
            if (!acceptorHandle)
                return;
            this.services[serviceId] = new PublishedService({
                remotePort: service.remotePort, 
                localPort: service.localPort,
                name: service.name ? service.name : serviceId, 
                acceptorHandle
            });
        })
    }

    private onConnect = (serviceId: string, socket: net.Socket) => {
        if (!this.services[serviceId])
        {
            console.log('Connection opened for unknown service', serviceId);
            return;
        }

        if (compareAddressStrings(socket.remoteAddress, this.publisherAddress))
        {
            console.log('Pipe connection from publisher open');
            this.services[serviceId].addSocketToAnyHalfOpenSession(socket);
            return;
        }

        console.log('New session for service', this.services[serviceId].name);
        const sessionId = generateUuid();
        this.services[serviceId].sessions[sessionId] = new Session(socket);
        socket.on('error', (error) => {
            console.log('Error in client socket for session Id', sessionId, error);
        })
        socket.on('close', () => {
            console.log('Session', sessionId, 'ended');
            if (this.services[serviceId])
                this.freeSessionForService(serviceId, sessionId);
        })
        this.controlSocket.send(JSON.stringify({
            type: 'NewSession',
            serviceId: serviceId,
            sessionId: sessionId,
            localPort: this.services[serviceId].localPort,
            remotePort: this.services[serviceId].remotePort 
        } as NewSessionMessage));
    }

    close = () => {
        this.clearServices();
        this.controlSocket.close();
    }
}

export default Publisher;