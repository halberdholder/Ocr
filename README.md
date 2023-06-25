# BUILD
Project depends on opencv-4.0.0 leptonica-1.83.1 tesseract-5.3.1,
Ensure that the C++ compilation toolchain is available, and install 
these package above first.

## Dependencies
* A compiler for C and C++: GCC or Clang
* cmake and make
* libjpeg-dev libpng-dev libtiff5-dev zlib1g-dev libwebpdemux2 libwebp-dev libopenjp2-7 libopenjp2-7-dev libcurl4-openssl-dev libgtk2.0-dev libavcodec-dev libavformat-dev libswscale-dev libavutil-dev libarchive-dev

```
> apt-get update
> apt-get install libjpeg-dev libpng-dev libtiff5-dev zlib1g-dev libwebpdemux2 libwebp-dev libopenjp2-7 libopenjp2-7-dev libcurl4-openssl-dev libgtk2.0-dev libavcodec-dev libavformat-dev libswscale-dev libavutil-dev libarchive-dev
```

Download there 3rd-party project sources from github, and build it.

## Build leptonica-1.83.1
```
> mkdir build_release && cd build_release
> cmake -DBUILD_SHARED_LIBS=OFF ..
> make [-j$(nproc)] && make install
```

## Build tesseract-5.3.1
```
> mkdir build_release && cd build_release
> cmake -DBUILD_SHARED_LIBS=OFF -DBUILD_TRAINING_TOOLS=OFF ..
> make [-j$(nproc)] && make install
```

## Build opencv-4.0.0
```
> mkdir build_release && cd build_release
> cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DOPENCV_FORCE_3RDPARTY_BUILD=ON -DOPENCV_GENERATE_PKGCONFIG=ON -DBUILD_TIFF=ON -DBUILD_JPEG=ON -DBUILD_PNG=ON -DBUILD_WEBP=ON -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
> make [-j$(nproc)] && make install
```

## Build this project
```
> mkdir build && cd build
```

Build release
```
> cmake -DCMAKE_BUILD_TYPE=Release ..
```

or build with debug info
```
> cmake -DCMAKE_BUILD_TYPE=Debug ..
```
```
> make [VERBOSE=1 | -j$(nproc)]
> make install
> make copy
```

The binary file and depends shared libaries all will be in dist directory.
