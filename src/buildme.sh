make clean
cd vldp2
chmod +x configure
./configure
make -f Makefile.linux
cd ../game/singe
make -f Makefile.linux
cd ../..
make
cd ..
echo .
echo done!
echo .
