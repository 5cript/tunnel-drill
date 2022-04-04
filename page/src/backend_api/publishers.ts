import * as easyFetch from '@tkrotoff/fetch';

class Publishers
{
    constructor() {
    }

    list = async () => {
        return easyFetch.get('/api/publishers');
    }
}

export default Publishers