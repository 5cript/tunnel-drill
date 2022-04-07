import net from 'net';
import dgram from 'dgram';
import { generateUuid } from './util/id';
import winston from 'winston';

interface SocketList<SocketType>
{
    [id: string]: SocketType;
}

interface AcceptorHandle
{
    id: string;
}
type MaybeAcceptorHandle = AcceptorHandle | undefined;

class TcpAcceptorPool
{
    limit: number;
    sockets: SocketList<net.Server>;
    
    constructor(limit: number)
    {
        this.limit = limit;
        this.sockets = {};
    }

    getAcceptors = () => {
        return Object.keys(this.sockets).map((key: string): AcceptorHandle => {return {id: key}});
    }

    makeAcceptor = (port: number, onConnection: (socket: net.Socket) => void): MaybeAcceptorHandle => {
        if (Object.keys(this.sockets).length >= this.limit) {
            return undefined;
        }
        const id = generateUuid();
        this.sockets[id] = net.createServer({
            allowHalfOpen: false,
            // TODO: might be useful.
            pauseOnConnect: false,
            // does not exist?
            // noDelay: true,
            // does not exist?
            // keepAlive: true,
        }, (socket: net.Socket) => {
            socket.on('error', (err) => {
                winston.error(`Socket error ${err.message}, ${JSON.stringify(err)}.`);
            })
            onConnection(socket);
        });

        this.sockets[id].on('error', (error: any) => {
            if (error.code === 'EADDRINUSE') {
                winston.error(`Address is already in use ${port}.`);
                this.freeAcceptor({id});
            }
        });

        this.sockets[id].listen(port);

        return {id};
    }

    freeAcceptor = (handle: AcceptorHandle) =>  {
        if (this.sockets[handle.id] === undefined)
        {
            winston.warn(`Acceptor for given handle ${handle.id} does not exist.`);
            return;
        }

        this.sockets[handle.id].close();
        this.sockets[handle.id].unref();
        delete this.sockets[handle.id];
    }

    freeAll = () => {
        this.getAcceptors().forEach(acceptor => {
            this.freeAcceptor(acceptor);
        })
    }
}

export {
    TcpAcceptorPool,
    AcceptorHandle
}