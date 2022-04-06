interface NewSessionMessage
{
    type: 'NewSession';
    serviceId: string;
    sessionId: string;
    localPort: number;
    remotePort: number;
}

interface NewSessionFailedMessage
{
    type: 'NewSessionFailed';
    localPort: number;
    remotePort: number;
    serviceId: string;
    sessionId: string;
    reason: string;
}

export {NewSessionMessage, NewSessionFailedMessage};