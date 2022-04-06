import { ServiceInfo } from "../service";

interface ServicesMessage
{
    type: 'Services';
    services: Array<ServiceInfo>
}

export {ServicesMessage};