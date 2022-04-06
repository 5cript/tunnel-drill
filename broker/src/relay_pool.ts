import dgram from 'dgram';

interface SocketList<SocketType>
{
    [id: string]: SocketType;
}

class UdpRelayPool 
{
    sockets: SocketList<dgram.Socket>;
}

export default UdpRelayPool;