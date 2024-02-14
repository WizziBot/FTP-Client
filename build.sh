#!/usr/bin/env bash

cd ./build
cmake ../
make
cd ..
cp ./build/Qt-GUI/Qt-GUI ./bin/
