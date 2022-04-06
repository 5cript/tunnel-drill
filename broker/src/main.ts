import os from 'os';
import TunnelBroker from './tunnel_broker';
import {default as pathTools} from 'path';
import fs from 'fs';
import Constants from './constants';
import Config from './config';

const main = () => {
    const home = os.homedir();
    const brokerHome = pathTools.join(home, Constants.specialHomeDirectory);
    const configPath = pathTools.join(brokerHome, 'config.json');
    let config: Config;
    try {
        config = JSON.parse(
            fs.readFileSync(configPath, {encoding: 'utf-8'})
        ) as Config;
    } catch (e) {
        console.log('Cannot read config file', configPath, e);
        return;
    }
    const broker = new TunnelBroker({
        key: fs.readFileSync(pathTools.join(brokerHome, 'key.pem'), {encoding: 'utf-8'}),
        cert: fs.readFileSync(pathTools.join(brokerHome, 'cert.pem'), {encoding: 'utf-8'}),
        config
    })

    console.log('Binding on', config.webSocketPort);
    broker.start(config.webSocketPort);
}

main();