interface UdpRelayBoundMessage
{
    type: 'UdpRelayBound';
    sessionId: string;
    serviceId: string;
    localPort: number;
    remotePort: number;
    socketType: string;
    relayPort: number;
}

export {UdpRelayBoundMessage};