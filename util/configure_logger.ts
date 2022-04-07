
import winston from 'winston';
const { combine, timestamp, label, printf } = winston.format;
import {default as pathTools} from 'path';
import isDev from './isdev';
import { generateId } from '../broker/src/util/id';

const configureLogger = (logFile: string) => 
{
    const myFormat = printf(({ level, message, label, timestamp } 
        : {level: any, message: string, label: string, timestamp: any}) => 
    {
        return `[${timestamp}](${label}) ${level}: ${message}`;
    });

    let logOutputs = [];
    if (isDev())
        logOutputs.push(new winston.transports.Console());
    else
        logOutputs.push(new winston.transports.File({filename: logFile}));
    
    winston.configure({
        format: combine(
            label({label: generateId(5)}),
            timestamp(),
            myFormat
        ),
        transports: logOutputs
    })
}

export default configureLogger;