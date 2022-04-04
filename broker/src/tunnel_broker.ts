import https from "https";
import express from "express";

class TunnelBroker
{
    app: express.Application;
    server: https.Server;

    constructor({key, cert}: {key: string, cert: string})
    {
        this.app = express();
        this.server = https.createServer({key, cert}, this.app);
        this.registerRoutes();
    }

    registerRoutes = () => {
        this.app.get('/', (req, res) => {
            
        });
    }

    start = (port: number) => {
        this.server.listen(port);
    }
}

export default TunnelBroker;