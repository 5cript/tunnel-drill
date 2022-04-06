import Publishers from './publishers';
import BackendBase from './backend_base';

class Backend extends BackendBase
{
    publishers: Publishers;

    constructor() {
        super();
        this.publishers = new Publishers(this);
    }
}

export default Backend;