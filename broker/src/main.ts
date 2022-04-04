import os from 'os';
import TunnelBroker from './tunnel_broker';
import {default as pathTools} from 'path';
import fs from 'fs';
import Constants from './consts';

const main = () => {
    const home = os.homedir();
    const brokerHome = pathTools.join(home, Constants.specialHomeDirectory);
    const broker = new TunnelBroker({
        key: fs.readFileSync(pathTools.join(brokerHome, 'key.pem'), {encoding: 'utf-8'}),
        cert: fs.readFileSync(pathTools.join(brokerHome, 'cert.pem'), {encoding: 'utf-8'}),
    })

    broker.start(11800);
}

main();