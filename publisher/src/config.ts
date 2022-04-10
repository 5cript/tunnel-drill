import { ServiceInfo } from '../../shared/service'

interface Config 
{
    identity: string;
    passHashed: string;
    host: string;
    port: number;
    authorityHost: string;
    authorityPort: number;
    services: Array<ServiceInfo>;
}

export default Config;