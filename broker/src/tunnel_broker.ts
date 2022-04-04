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
        this.app.use('/frontend', express.static('app'));
        this.app.get('/', (req, res) => {
            res.redirect('/frontend/index.html');
        })
        this.app.get('/api/publishers')
    }

    start = (port: number) => {
        this.server.listen(port);
    }
}

export default TunnelBroker;