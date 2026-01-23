const path = require('path');

module.exports = {
    target: 'node', // 关键：指定为 Node.js 环境
    mode: 'production',
    entry: './src/index.js',
    output: {
        filename: 'index.js',
        path: path.resolve(__dirname, 'dist'),
    },
    resolve: {
        modules: ['node_modules', path.resolve(__dirname, 'node_modules')],
    },
    module: {
    },
};