import os from 'os';
import Publisher from "./publisher";
import {default as pathTools} from 'path';
import fs from 'fs';
import Constants from './constants';
import Config from './config';

const main = () => {
    const home = os.homedir();
    const publisherHome = pathTools.join(home, Constants.specialHomeDirectory);
    const config = JSON.parse(
        fs.readFileSync(pathTools.join(publisherHome, 'config.json'), {encoding: 'utf-8'})
    ) as Config;
    const publisher = new Publisher(
        "wss://" + config.host + ':' + config.port + "/api/ws/publisher", 
        config.services, 
        config.host
    );

    let end = false;
    process.on('SIGINT', () => {
        end = true;
    });
    const wait = () => {
        if (!end) 
            setTimeout(wait, 1000);
        else {
            console.log('Ending publisher');
            publisher.stop();
        }
    };
    wait();
}

main();