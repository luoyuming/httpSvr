rm -rf build

mkdir -p build
cd  build

rm -rf *

cp ../*.xml		./
cp ../*.json	./
cp ../*.html	./

mkdir -p wwwroot
cp -r ../../wwwroot/*  ./wwwroot/
mkdir -p www.qytmail.com
cp -r ../../www.qytmail.com/*  ./www.qytmail.com/

cp  ../*.php  ./
chmod +777  *.php
cmake -DCMAKE_BUILD_TYPE=Debug ..
make



