import Publishers from './publishers'

class Backend
{
    publishers: Publishers;

    constructor() {
        this.publishers = new Publishers();
    }
}

export default Backend;