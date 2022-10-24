import { writable } from 'svelte/store';

export const sessionTokenPayloadDefault = {
    exp: 0,
    iat: 0,
    identity: null,
    iss: null,
};
export const sessionTokenPayload = writable(sessionTokenPayloadDefault);