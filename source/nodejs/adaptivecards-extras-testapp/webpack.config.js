const path = require("path");
const HtmlWebpackPlugin = require('html-webpack-plugin');

module.exports = (env, argv) => {
	const mode = argv.mode || 'development';
	const devMode = mode === "development";

	console.info('running webpack with mode:', mode);

	return {
		mode: mode,
		entry: {
			"adaptivecards-extras-testapp": "./src/adaptivecards-extras-testapp.ts"
		},
		output: {
			path: path.resolve(__dirname, "./dist"),
			filename: devMode ? "[name].js" : "[name].min.js",
			library: "ACExtrasTestApp",
			libraryTarget: "umd",
			globalObject: "this",
			// umdNamedDefine: true
		},
		devtool: devMode ? "inline-source-map" : "source-map",
		devServer: {
			contentBase: './dist'
		},
		resolve: {
			extensions: [".ts", ".tsx", ".js"]
		},
		module: {
			rules: [
				{
					test: /\.ts$/,
					loader: "ts-loader",
					exclude: /(node_modules|__tests__)/
				}
			]
		},
		plugins: [
            new HtmlWebpackPlugin({
				title: "Adataptive Cards Extras test application",
				template: "./index.html"
			})
		],
		externals: {
			"adaptivecards": {
				commonjs2: "adaptivecards",
				commonjs: "adaptivecards",
				root: "AdaptiveCards"
			},
			"adaptivecards-extras": {
				commonjs2: "adaptivecards-extras",
				commonjs: "adaptivecards-extras",
				root: "ACExtras"
			}
		}
	}
}
