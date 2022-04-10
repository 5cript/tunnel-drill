interface ServiceInfo
{
    name?: string;
    socketType: string;
    hiddenPort: number;
    publicPort: number;
    hiddenHost?: string;
}

export {
    ServiceInfo
}