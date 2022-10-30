import WebSocket from "ws";
import RetryContext from "../../util/retry";
import { ServiceInfo } from '../../shared/service';
import { NewTunnelMessage, NewTunnelFailedMessage } from "../../shared/control_messages/new_tunnel";
import https from 'https';
import http from 'http';
import { generateUuid } from "../../broker/src/util/id";
import TcpSession from "./tcp_session";
import UdpSession from "./udp_session";
import { ServicesMessage } from "../../shared/control_messages/services";
import { UdpRelayBoundMessage } from "../../shared/control_messages/udp";
import winston from 'winston';
import { HandshakeMessage } from "../../shared/control_messages/handshake";
import jwt from 'jsonwebtoken';
import Config from "./config";

const p2bTunnelPrefix = "TUNNEL_BORE_P2B";

class LocalService implements ServiceInfo
{
    name?: string;
    hiddenPort: number;
    publicPort: number;
    socketType: string;
    hiddenHost?: string;
    // TODO: improve only one is needed.
    tcpSessions: {[id: string]: TcpSession}
    udpSessions: {[id: string]: UdpSession}

    constructor({publicPort, hiddenPort, socketType, name, hiddenHost}: 
        {publicPort: number, hiddenPort: number, socketType: string, name?: string, hiddenHost?: string}) 
    {
        this.name = name;
        this.hiddenHost = hiddenHost;
        this.hiddenPort = hiddenPort;
        this.publicPort = publicPort;
        this.socketType = socketType.toLowerCase();
        this.tcpSessions = {};
        this.udpSessions = {};
    }

    createSession = (brokerHost: string, token: any, onBound: (port: number) => void) => {
        const id = generateUuid();
        if (this.socketType === 'tcp')
        {
            this.tcpSessions[id] = new TcpSession(this.publicPort, this.hiddenPort, brokerHost, p2bTunnelPrefix + ':' + token, () => {
                this.freeSession(id);
            }, this.hiddenHost);
        }
        else
        {
            this.udpSessions[id] = new UdpSession({
                publicPort: this.publicPort, 
                hiddenPort: this.hiddenPort, 
                brokerHost, 
                socketType: this.socketType, 
                token: p2bTunnelPrefix + ':' + token, 
                onAnyClose: () => {
                    this.freeSession(id);
                },
                onBound
            });
        }
        return id;
    }

    freeSession = (id: string) => {
        if (this.socketType === 'tcp')
        {
            if (this.tcpSessions[id] === undefined)
                return;
            
            this.tcpSessions[id].free();
            delete this.tcpSessions[id];
        }
        else
        {
            if (this.udpSessions[id] === undefined)
                return;
            
            this.udpSessions[id].free();
            delete this.udpSessions[id];
        }
    }

    freeAll = () => {
        for (const tunnelId in this.tcpSessions) {
            this.freeSession(tunnelId);
        }
    }
}

class Publisher
{
    ws: WebSocket;
    connectRepeater: RetryContext;
    services: {[hiddenPort: number]: LocalService};
    messageMap: {[messageName: string]: (...args: any) => void}
    brokerHost: string;
    shallDie: boolean;
    identity: string;
    authToken: string;
    config: Config;

    constructor(wsPath: string, config: Config) {
        this.identity = config.identity;
        this.brokerHost = config.host;
        this.services = {};
        this.messageMap = {};
        this.authToken = '';
        this.config = config;

        config.services.forEach(service => {
            this.services[service.hiddenPort] = new LocalService({
                hiddenPort: service.hiddenPort, 
                publicPort: service.publicPort,
                socketType: service.socketType,
                name: service.name,
                hiddenHost: service.hiddenHost,
            });
        })
        this.shallDie = false;

        // register understood messages:
        this.messageMap["NewTunnel"] = (messageObject: NewTunnelMessage) => {
            this.onNewTunnel(messageObject);
        }
        this.messageMap["Error"] = (messageObject: any) => {
            winston.error(`Error from broker: ${messageObject.message}`);
        }

        this.connectRepeater = new RetryContext(() => {this.connect(this.authToken, wsPath);}, 1000);

        const req = https.request({
            hostname: config.authorityHost,
            port: config.authorityPort,
            path: '/api/auth',
            method: 'GET',
            auth: this.config.identity + ':' + this.config.passHashed,
            rejectUnauthorized: false
        }, res => {
            if (res.statusCode !== 200)
            {
                winston.error("Could not authenticate with authority.");
                process.exit(1);
            }
            res.on('data', token => {
                this.authToken = token;
                this.connect(this.authToken, wsPath);
            })
        });
        
        req.on('error', error => {
            winston.error(`Error in authentication request ${error.message}`);
            process.exit(1);
        })

        req.end();
    }

    stop = () => {
        this.shallDie = true;
        this.connectRepeater.abort();
        this.ws.close();
        for (const serviceId in this.services) {
            this.services[serviceId].freeAll();
        }
    }

    private onNewTunnel = ({tunnelId, serviceId, hiddenPort, publicPort, socketType}: NewTunnelMessage) => {
        winston.info(`New session for ${hiddenPort}.`);

        const replyWithFailure = (reason: string) => {
            const reply: NewTunnelFailedMessage = {
                type: "NewTunnelFailed", tunnelId, serviceId, hiddenPort, publicPort, reason, socketType
            }
            this.ws.send(JSON.stringify(reply));
        }

        if (!this.services[hiddenPort]) {
            winston.error(`There is no service for that port ${hiddenPort}`);
            replyWithFailure('There is no service here for that port.');
        }

        const onBound = (port: number) => {
            this.ws.send(JSON.stringify({
                type: "UdpRelayBound",
                tunnelId,
                serviceId,
                hiddenPort,
                publicPort,
                socketType,
                relayPort: port
            } as UdpRelayBoundMessage))
        }
        try {
            const req = https.request({
                hostname: this.config.authorityHost,
                port: this.config.authorityPort,
                path: '/api/auth/sign-json',
                method: 'POST',
                rejectUnauthorized: false,
                auth: this.config.identity + ':' + this.config.passHashed,
            }, res => {
                if (res.statusCode !== 200)
                {
                    winston.error(`Could not authenticate with authority to sign p2b tunnel. (Status code: ${res.statusCode})`);
                    process.exit(1);
                }
                res.on('data', tokenJson => {
                    const token = JSON.parse(tokenJson).token;
                    this.services[hiddenPort].createSession(this.brokerHost, token, onBound);
                })
            });
            
            req.on('error', error => {
                winston.error(`Error in authentication request ${error.message}`);
                process.exit(1);
            })
            req.write(JSON.stringify({tunnelId, serviceId, hiddenPort, publicPort}));    
            req.end();
        }
        catch (error) {
            replyWithFailure(error.message);
            winston.error(`Could not create session ${tunnelId}.`);
        }
    }

    private connect = (token: string, wsPath: string) => {
        if (this.shallDie)
            return;

        winston.info(`Trying to connect to broker ${wsPath}`);
        try {
            this.ws = new WebSocket(wsPath, {
                rejectUnauthorized: false,
                headers: {
                    'Authorization': 'Bearer ' + Buffer.from(token).toString('base64')
                }
            });
        } catch (error) {
            winston.error(`Exception in control line connect ${error.message}`);
            return;
        }

        this.ws.on('error', (error: any) => {
            if (error.code === 'ECONNREFUSED')
            {
                winston.info('Broker is offline.');
            }
            else 
            {
                winston.error(`Error on control connection ${error.message}`);
            }
        })

        this.ws.on('close', () => {
            if (!this.shallDie) {
                winston.warn(`Connection to broker lost, retrying in ${this.connectRepeater.currentTime / 1000} seconds`);
                this.connectRepeater.retry();
            }
        })

        this.ws.on('open', () => {
            winston.info('Connected to broker.');
            this.connectRepeater.reset();
            this.ws.send(JSON.stringify({
                type: "Handshake",
                identity: this.identity,
                services: Object.keys(this.services).map((servicePort: string) => {
                    return {
                        hiddenPort: this.services[parseInt(servicePort)].hiddenPort, 
                        publicPort: this.services[parseInt(servicePort)].publicPort,
                        name: this.services[parseInt(servicePort)].name
                    }
                })
            } as HandshakeMessage));
        })

        this.ws.on('message', (data, isBinary: boolean) => {
            if (!isBinary)
            {
                const msg = JSON.parse(data.toString());
                const messageHandler = this.messageMap[msg.type];
                if (!messageHandler) {
                    winston.error(`There is no message handler for ${msg.type}`);
                    return;
                }
                messageHandler(msg);
            }
        })
    }
};

export default Publisher;