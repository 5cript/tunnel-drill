import os from 'os';
import TunnelBroker from './tunnel_broker';
import {default as pathTools} from 'path';
import fs from 'fs';
import Constants from './constants';
import Config from './config';
import isDev from '../../util/isdev';
import configureLogger from '../../util/configure_logger';
import winston from 'winston';

const main = () => {
    const home = os.homedir();
    const brokerHome = pathTools.join(home, Constants.specialHomeDirectory);

    configureLogger(pathTools.join(brokerHome, "combined.log"));

    let configPath = pathTools.join(brokerHome, 'config.json');
    if (isDev())
        configPath = pathTools.join(brokerHome, 'configDev.json');
    let config: Config;
    try {
        config = JSON.parse(
            fs.readFileSync(configPath, {encoding: 'utf-8'})
        ) as Config;
    } catch (e) {
        winston.error(`Cannot read config file at ${configPath} with error ${e.message}`);
        return;
    }
    const broker = new TunnelBroker({
        key: fs.readFileSync(pathTools.join(brokerHome, 'key.pem'), {encoding: 'utf-8'}),
        cert: fs.readFileSync(pathTools.join(brokerHome, 'cert.pem'), {encoding: 'utf-8'}),
        config
    })

    winston.info(`Binding on ${config.webSocketPort}`);
    if (!isDev())
        console.log(`Binding on ${config.webSocketPort}`);
        
    broker.start(config.webSocketPort);
}

main();