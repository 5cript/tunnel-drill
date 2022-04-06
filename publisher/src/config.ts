import { ServiceInfo } from '../../shared/service'

interface Config 
{
    host: string;
    port: number;
    services: Array<ServiceInfo>;
}

export default Config;