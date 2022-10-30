import os from 'os';
import Publisher from "./publisher";
import {default as pathTools} from 'path';
import fs from 'fs';
import Constants from './constants';
import Config from './config';
import isDev from "../../util/isdev";
import configureLogger from '../../util/configure_logger';
import winston from 'winston';

const main = () => {
    const home = os.homedir();
    const publisherHome = pathTools.join(home, Constants.specialHomeDirectory);
    
    configureLogger(pathTools.join(publisherHome, "combined.log"));

    let configPath = pathTools.join(publisherHome, 'config.json');
    if (isDev())
        configPath = pathTools.join(publisherHome, 'configDev.json');

    let publisher: Publisher;
    try {
        const config = JSON.parse(
            fs.readFileSync(configPath, {encoding: 'utf-8'})
        ) as Config;
        winston.error("Config: ", config);
        publisher = new Publisher(
            "wss://" + config.host + ':' + config.port + "/api/ws/publisher", 
            config
        );
    } catch (e) {
        winston.error("Failed to load config file and create publisher: " + e);
    }

    let end = false;
    process.on('SIGINT', () => {
        end = true;
    });
    const wait = () => {
        if (!end) 
            setTimeout(wait, 1000);
        else {
            winston.info('Ending publisher.');
            publisher.stop();
        }
    };
    wait();
}

main();