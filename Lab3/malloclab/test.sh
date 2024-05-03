#! /bin/bash
FITMODE=$1
cd ../trace/; sh test.sh $FITMODE
#cd ../malloclab/ ; sudo cp libmem.so /usr/lib/
#cp mm.h ../trace/
#cp memlib.h ../trace/
#cp config.h ../trace/

# sudo rm /usr/lib/libmem.so