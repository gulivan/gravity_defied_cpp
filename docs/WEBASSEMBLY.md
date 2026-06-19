# WebAssembly build

The browser build uses Emscripten, SDL2, SDL2_image, SDL2_ttf, and the existing embedded assets. The output is static and can be hosted by GitHub Pages.

## Local build

Install and activate Emscripten first:

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

Build the web target from this repository:

```sh
emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --target GravityDefied
```

The generated files are:

```text
build-web/GravityDefied.html
build-web/GravityDefied.js
build-web/GravityDefied.wasm
```

Serve the directory with any static web server:

```sh
python3 -m http.server 8000 --directory build-web
```

Then open:

```text
http://localhost:8000/GravityDefied.html
```

## GitHub Pages

This repo includes `.github/workflows/pages.yml`, which builds the WebAssembly target and publishes it with GitHub Pages. In the repository settings, set Pages to use "GitHub Actions", then run the "Deploy WebAssembly to Pages" workflow or push to `main`/`master`.

For manual GitHub Pages publishing, publish the three generated files from `build-web`. If Pages serves from a branch or `/docs`, rename `GravityDefied.html` to `index.html` in the published directory.

The game stores record data through Emscripten IDBFS under `/gravity_defied`, backed by browser IndexedDB. Saves are local to the browser profile and origin.
