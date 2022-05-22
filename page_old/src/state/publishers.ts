import {
    atom,
    selector,
} from 'recoil';

const publishersState = atom({
    key: 'users',
    default: []
})

export {publishersState}