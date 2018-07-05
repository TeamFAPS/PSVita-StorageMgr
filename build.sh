cd kernel
rm -rf build
mkdir build
cd build
cmake ..
make clean
make install
cd ../..