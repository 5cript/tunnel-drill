interface UdpRelayBoundMessage
{
    type: 'UdpRelayBound';
    tunnelId: string;
    serviceId: string;
    hiddenPort: number;
    publicPort: number;
    socketType: string;
    relayPort: number;
}

export {UdpRelayBoundMessage};