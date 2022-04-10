interface NewTunnelMessage
{
    type: 'NewTunnel';
    serviceId: string;
    tunnelId: string;
    hiddenPort: number;
    publicPort: number;
    socketType: string;
}

interface NewTunnelFailedMessage
{
    type: 'NewTunnelFailed';
    hiddenPort: number;
    publicPort: number;
    serviceId: string;
    tunnelId: string;
    reason: string;
    socketType: string;
}

export {NewTunnelMessage, NewTunnelFailedMessage};