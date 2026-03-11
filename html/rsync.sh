#!/bin/bash
set -euxo pipefail

rsync -vaP cryptominisat5.js cryptominisat5.wasm  index.html msoos.org:/var/www/cryptominisat/
