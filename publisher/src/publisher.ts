import WebSocket from "ws";
import RetryContext from "../../util/retry";
import { ServiceInfo } from '../../shared/service';
import { NewSessionMessage, NewSessionFailedMessage } from "../../shared/control_messages/new_session";
import net from 'net';
import { generateUuid } from "../../broker/src/util/id";
import Piper from "../../shared/piper";
import { ServicesMessage } from "../../shared/control_messages/services";

class TcpSession extends Piper
{};

class LocalService implements ServiceInfo
{
    name?: string;
    localPort: number;
    remotePort: number;
    sessions: {[id: string]: TcpSession}

    constructor({remotePort, localPort, name}: {remotePort: number, localPort: number, name?: string}) 
    {
        this.name = name;
        this.localPort = localPort;
        this.remotePort = remotePort;
        this.sessions = {};
    }

    createSession = (brokerHost: string, token: any) => {
        const id = generateUuid();
        this.sessions[id] = new TcpSession(this.remotePort, this.localPort, brokerHost, JSON.stringify(token), () => {
            this.freeSession(id);
        });
        return id;
    }

    freeSession = (id: string) => {
        if (this.sessions[id] === undefined)
            return;
        
        this.sessions[id].free();
        delete this.sessions[id];
    }

    freeAll = () => {
        for (const sessionId in this.sessions) {
            this.freeSession(sessionId);
        }
    }
}

class Publisher
{
    ws: WebSocket;
    connectRepeater: RetryContext;
    services: {[localPort: number]: LocalService};
    messageMap: {[messageName: string]: (...args: any) => void}
    brokerHost: string;
    shallDie: boolean;

    constructor(wsPath: string, services: Array<ServiceInfo>, brokerHost: string) {
        this.brokerHost = brokerHost;
        this.services = {};
        this.messageMap = {};
        services.forEach(service => {
            this.services[service.localPort] = new LocalService({
                localPort: service.localPort, 
                remotePort: service.remotePort,
                name: service.name
            });
        })
        this.shallDie = false;

        // register understood messages:
        this.messageMap["NewSession"] = (messageObject: NewSessionMessage) => {
            console.log(messageObject);
            this.onNewSession(messageObject);
        }
        
        this.connectRepeater = new RetryContext(() => {this.connect(wsPath);}, 1000);
        this.connect(wsPath);
    }

    stop = () => {
        this.shallDie = true;
        this.connectRepeater.abort();
        this.ws.close();
        for (const serviceId in this.services) {
            this.services[serviceId].freeAll();
        }
    }

    private onNewSession = ({sessionId, serviceId, localPort, remotePort}: {
        sessionId: string,
        serviceId: string,
        localPort: number,
        remotePort: number,
    }) => {
        console.log('New session for', localPort);

        const replyWithFailure = (reason: string) => {
            const reply: NewSessionFailedMessage = {
                type: "NewSessionFailed", sessionId, serviceId, localPort, remotePort, reason
            }
            this.ws.send(JSON.stringify(reply));
        }

        if (!this.services[localPort]) {
            console.log('There is no service for that port', localPort);
            replyWithFailure('There is no service here for that port.');
        }

        try {
            this.services[localPort].createSession(this.brokerHost, {sessionId, serviceId, localPort, remotePort});
        }
        catch (error) {
            replyWithFailure(error.message);
            console.log('Error in pipe builtup', error);
        }
    }

    private connect = (wsPath: string) => {
        if (this.shallDie)
            return;

        console.log("Trying to connect to broker", wsPath);
        try {
            this.ws = new WebSocket(wsPath, {
                rejectUnauthorized: false
            });
        } catch (error) {
            console.log(error);
            return;
        }

        this.ws.on('error', (error: any) => {
            if (error.code === 'ECONNREFUSED')
            {
                console.log('Broker is offline');
            }
            else 
            {
                console.log('Error on control connection', error);
            }
        })

        this.ws.on('close', () => {
            if (!this.shallDie) {
                console.log(`Connection to broker lost, retrying in ${this.connectRepeater.currentTime / 1000} seconds`);
                this.connectRepeater.retry();
            }
        })

        this.ws.on('open', () => {
            console.log('Connected to broker');
            this.connectRepeater.reset();
            this.ws.send(JSON.stringify({
                type: "Services",
                services: Object.keys(this.services).map((servicePort: string) => {
                    return {
                        localPort: this.services[parseInt(servicePort)].localPort, 
                        remotePort: this.services[parseInt(servicePort)].remotePort,
                        name: this.services[parseInt(servicePort)].name
                    }
                })
            } as ServicesMessage));
        })

        this.ws.on('message', (data, isBinary: boolean) => {
            if (!isBinary)
            {
                const msg = JSON.parse(data.toString());
                const messageHandler = this.messageMap[msg.type];
                if (!messageHandler) {
                    console.log('There is no message handler for', msg.type, msg);
                    return;
                }
                messageHandler(msg);
            }
        })
    }
};

export default Publisher;