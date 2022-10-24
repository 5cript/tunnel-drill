import { sessionTokenPayload, sessionTokenPayloadDefault } from "./stores.js";
import jwt_decode from "jwt-decode";

let token = sessionTokenPayloadDefault;
const tokenSubscription = sessionTokenPayload.subscribe(value => {
    token = value;
})

const isLoggedIn = () => {
    return token.identity !== null && Math.floor(Date.now() / 1000) < token.exp;
}

const setSession = (token) => {
    localStorage.setItem("sessionToken", token);
    sessionTokenPayload.update(() => jwt_decode(token));
}

const logout = () => {
    sessionTokenPayload.update(() => sessionTokenPayloadDefault);
}

export { isLoggedIn, setSession, logout };
