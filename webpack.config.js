import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default {
  target: 'node',
  entry: {
    broker: { import: './broker/src/main.ts', filename: 'broker.cjs' },
    publisher: { import: './publisher/src/main.ts', filename: 'publisher.cjs' }
  },
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        use: 'ts-loader',
        exclude: /node_modules/,
      },
    ],
  },
  resolve: {
    extensions: ['.tsx', '.ts', '.js'],
  },
  output: {
    path: path.resolve(__dirname, 'dist')
  }
};