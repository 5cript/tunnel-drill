import { ServiceInfo } from "../service";

interface HandshakeMessage
{
    type: 'Handshake';
    identity: string;
    services: Array<ServiceInfo>
}

export {HandshakeMessage};