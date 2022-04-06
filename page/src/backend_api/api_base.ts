import BackendBase from "./backend_base";

class ApiBase
{
    backend: BackendBase;

    constructor(backend: BackendBase)  {
        this.backend = backend;
    }

    url = (relative: string) => {
        return window.location.origin + relative
    }
}

export default ApiBase;