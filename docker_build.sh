#! /bin/bash

cd /home/kapz/px4/MAVSDK
docker run --rm dockcross/linux-armv7-lts > ./dockcross-linux-armv7-lts
chmod +x ./dockcross-linux-armv7-lts
./dockcross-linux-armv7-lts cmake -Bbuild/linux-armv7 -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -H.
./dockcross-linux-armv7-lts cmake --build build/linux-armv7 -j 8 --target install

cd /home/kapz/px4/MavPiCam
docker run --rm dockcross/linux-armv7 > ./dockcross-linux-armv7-lts
chmod +x ./dockcross-linux-armv7-lts
./dockcross-linux-armv7-lts  -a "-v /home/kapz/px4/MAVSDK:/home/kapz/px4/MAVSDK" cmake -DCMAKE_PREFIX_PATH=/home/kapz/px4/MAVSDK/install -Bbuild -H.
./dockcross-linux-armv7-lts -a "-v /home/kapz/px4/MAVSDK:/home/kapz/px4/MAVSDK" cmake --build build -j8