rm -rf build

mkdir -p build
cd  build

rm -rf *

cp ../*.xml		./
cp ../*.json	./
cp ../*.html	./


cp  ../*.php  ./
chmod +777  *.php
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
./httpSvr


