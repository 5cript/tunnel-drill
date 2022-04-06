interface NewSessionMessage
{
    type: 'NewSession';
    serviceId: string;
    sessionId: string;
    localPort: number;
    remotePort: number;
    socketType: string;
}

interface NewSessionFailedMessage
{
    type: 'NewSessionFailed';
    localPort: number;
    remotePort: number;
    serviceId: string;
    sessionId: string;
    reason: string;
    socketType: string;
}

export {NewSessionMessage, NewSessionFailedMessage};