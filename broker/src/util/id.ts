import { v4 as uuidv4 } from 'uuid';

const generateId = (length: number) => {
    let id = '';
    const lookup = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
    for (let i = 0; i !== length; ++i) 
        id += lookup[Math.floor(Math.random() * lookup.length)];
    return id;
}

const generateUuid = () => {
    return uuidv4();
}

export {generateId, generateUuid};