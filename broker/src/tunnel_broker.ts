import https from "https";
import http from "http";
import express from "express";
import Publisher from "./publisher";
import { WebSocketServer, WebSocket } from 'ws';
import { generateUuid } from './util/id';
import { ServiceInfo } from '../../shared/service';
import Config from './config';
import { TcpAcceptorPool } from "./acceptor_pool";
import winston from 'winston';

interface FrontendSession
{
    socket: WebSocket;
}

class TunnelBroker
{
    app: express.Application;
    server: https.Server;
    webSocket: WebSocketServer;
    frontendSessions: Map<string, FrontendSession>;
    publishers: Map<string, Publisher>;
    config: Config;  
    // FIXME: not correctly implemented, is now per publisher and overall is unlimited.
    remainingLimit: number; 

    constructor({key, cert, config}: {key: string, cert: string, config: Config})
    {
        this.config = config;
        this.app = express();
        this.server = https.createServer({key, cert}, this.app);
        this.webSocket = new WebSocketServer({server: this.server});
        this.publishers = new Map<string, Publisher>();
        this.frontendSessions = new Map<string, FrontendSession>();
        this.remainingLimit = config.maximumTcpPorts;
        
        this.registerRoutes();
        this.setupWebSocket();
    }

    private setupWebSocket = () => {
        this.webSocket.on('connection', (socket: WebSocket, request) => {
            if (request.url === '/api/ws/frontend') {
                winston.info('New frontend session.');
                this.setupFrontendSession(socket, request);
            } else if (request.url === '/api/ws/publisher') {
                this.setupPublisherSession(socket, request);
            }
        })
    }

    private setupFrontendSession = (socket: WebSocket, request: http.IncomingMessage) =>
    {
        const id = generateUuid();
        this.frontendSessions.set(id, {
            socket
        });
        socket.on('close', () => {
            this.frontendSessions.delete(id);
        })
        socket.on('message', (data, isBinary) => {
            if (!isBinary) {
                winston.debug(`Message from frontend with id ${id}: ${data.toString('utf-8')}`);
            }
        })
    }

    private setupPublisherSession = (socket: WebSocket, request: http.IncomingMessage) =>
    {
        const id = generateUuid();
        winston.info(`New publisher connected with id ${id}`);
        this.publishers.set(id, new Publisher(socket, request.socket.remoteAddress, this.remainingLimit));
        socket.on('close', () => {
            winston.info(`Publisher lost connection with id ${id}`);
            this.publishers.get(id).close();
            this.publishers.delete(id);
        })
    }

    private registerRoutes = () => {
        this.app.get('/', (req, res) => {
            res.redirect('/frontend/index.html');
        })
        this.app.use('/frontend', express.static('app'));
        this.app.get('/api/publishers', (req, res) => {
            res.send(this.publishers);
        })
    }

    start = (port: number) => {
        this.server.listen(port);
    }
}

export default TunnelBroker;