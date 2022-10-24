import './styles/theme.scss';
import App from './app.svelte';
import { setSession } from './session';

const token = localStorage.getItem("sessionToken");
if (token) {
    setSession(token);
}

const app = new App({
    target: document.body,
    props: {
        name: 'world'
    }
});

export default app;