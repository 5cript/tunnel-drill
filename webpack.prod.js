import { fileURLToPath } from 'url';
import path from 'path';
import { merge } from 'webpack-merge';
import common from './webpack.common.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default merge(common, {
    mode: 'production',
    output: {
      path: path.resolve(__dirname, 'prod')
    },
});