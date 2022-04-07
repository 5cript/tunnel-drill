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
  ignoreWarnings: [
    {
      module: /node_modules\/express\/lib\/view\.js/,
      message: /the request of a dependency is an expression/,
    }
  ],
  experiments: {
    topLevelAwait: true
  }
};