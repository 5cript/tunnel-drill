import * as easyFetch from '@tkrotoff/fetch';
import ApiBase from './api_base';
import BackendBase from './backend_base';

class Publishers extends ApiBase
{
    constructor(backend: BackendBase) {
        super(backend);
    }

    list = async () => {
        return easyFetch.get(this.url('/api/publishers')).json();
    }
}

export default Publishers