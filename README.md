# Consensus-Algorithm

## Contents
- [Environment](#environment)
- [Installation](#installation)
  - [NS-3](#ns-3)
  - [OpenSSL](#openssl)
  - [liboqs](#liboqs)
- [Getting Started](#getting-started)
  - [1. Add Library (liboqs, OpenSSL)](#1-add-library-liboqs-openssl)
  - [2. Blockchain Simulator](#2-blockchain-simulator)
  - [3. Network Helper](#3-network-helper)
  - [4. Consensus Algorithm](#4-consensus-algorithm)
  - [5. Run Simulator](#5-run-simulator)
  - [6. Change Algorithm](#6-change-algorithm)
- [Reference](#Reference)
- [Contributor](#Contributor)
- [License](#License)


## Environment
- __Processor__: Intel NUC(Intel i3-6100U CPU)
- __RAM__: 16GB
- __OS__: Ubuntu 20.04.6 LTS
- __Simulator__: NS-3 (Network Simulator)
  - __Version__: ns-3.38-dirty
- __Language__: C++

## Installation
  ### NS-3
  
    git clone https://gitlab.com/nsnam/ns-3-dev.git
    cd ns-3-dev
    git checkout -b ns-3.38-branch ns-3.38

---
  
  ### OpenSSL

  install dependencies.
  
    sudo apt-get update
    sudo apt-get install -y build-essential git cmake python3 libssl-dev

  clone `liboqs` repo.

    git clone https://github.com/open-quantum-safe/liboqs.git

  install `liboqs`.

    cd liboqs
    mkdir build
    cd build
    cmake -GNinja -DOQS_USE_OPENSSL=1 ..
    ninja
    sudo ninja install

  clone `OQS-OpenSSL` repo.

    cd ~
    git clone https://github.com/open-quantum-safe/openssl.git

  build `OQS-OpenSSL`.

    cd openssl
    ./Configure shared --prefix=/usr/local/ssl --openssldir=/usr/local/ssl linux-x86_64
    make -j
    sudo make install

  update `LD_LIBRARY_PATH` environment variable.
  
    export LD_LIBRARY_PATH=/usr/local/ssl/lib:$LD_LIBRARY_PATH

  if you do not have `liboqs.so` in `/usr/local/lib`

    cd ~/openssl
    ./Configure shared --prefix=/usr/local/ssl --openssldir=/usr/local/ssl linux-x86_64 -I/usr/local/include -L/usr/local/lib -DOQS_DIR=/usr/local
    make -j
    sudo make install

---
  
  ### liboqs

  install `liboqs`

    sudo apt install astyle cmake gcc ninja-build libssl-dev python3-pytest python3-pytest-xdist unzip xsltproc doxygen graphviz python3-yaml valgrind
    cd ~
    git clone -b main https://github.com/openquantum-safe/liboqs.git
    cd liboqs
    mkdir build && cd build
    cmake -GNinja -DBUILD_SHARED_LIBS=ON ..
    ninja

  
## Getting Started
  ### 1. Add Library (liboqs, OpenSSL)
  edit `CMakeLists.txt` in `scratch` folder
  
  before

    # Get source absolute path and transform into relative path
      get_filename_component(scratch_src ${scratch_src} ABSOLUTE)
      get_filename_component(scratch_absolute_directory ${scratch_src} DIRECTORY)
      string(REPLACE "${PROJECT_SOURCE_DIR}" "${CMAKE_OUTPUT_DIRECTORY}"
        scratch_directory ${scratch_absolute_directory}
        )
    
    build_exec(
      EXECNAME ${scratch_name}
      EXECNAME_PREFIX ${target_prefix}
    	SOURCE_FILES "${source_files}"
    	LIBRARIES_TO_LINK "${ns3-libs}" "${ns3-contrib-libs}"
    	EXECUTABLE_DIRECTORY_PATH ${scratch_directory}/
  	)

   after

     # Get source absolute path and transform into relative path
       get_filename_component(scratch_src ${scratch_src} ABSOLUTE)
       get_filename_component(scratch_absolute_directory ${scratch_src} DIRECTORY)
       string(REPLACE "${PROJECT_SOURCE_DIR}" "${CMAKE_OUTPUT_DIRECTORY}"
         scratch_directory ${scratch_absolute_directory}
         )
  
     find_external_library(
       DEPENDENCY_NAME oqs
       HEADER_NAMES oqs.h
       LIBRARY_NAMES oqs
       SEARCH_PATHS "~/liboqs/build/lib/"
     )
  
    set(include_dir "~/liboqs/build/include/")
    include_directories(${include_dir})
    link_libraries(${oqs_LIBRARIES})
    
    find_package(OpenSSL)
  
    build_exec(
  	  EXECNAME ${scratch_name}
  	  EXECNAME_PREFIX ${target_prefix}
  	  SOURCE_FILES "${source_files}"
  	  LIBRARIES_TO_LINK "${ns3-libs}" "${ns3-contrib-libs}" "${OPENSSL_LIBRARIES}" "${oqs_LIBRARIES}"
  	  EXECUTABLE_DIRECTORY_PATH ${scratch_directory}/
  	)

  ### 2. Blockchain Simulator
  Move `blockchain-simulator.cc` from `scratch` into `ns-3-dev/scratch`
    

  ### 3. Network Helper
  1. Move `network-helper.cc` and `network.helper.h` from `network-helper` into `ns-3-dev/src/application/helper`

  2. Change the /ns-3-dev/src/application/CMakeLists.txt file
  
  Add the following to the `SOURCE_FILES` list:

    helper/network-helper.cc
    
  Add the following to the `HEADER_FILES` list:

    helper/network-helper.h
        
  ### 4. Consensus Algorithm
  1. Move `pow.cc` and `pow.h`(or you want) from `PQC_algorithms/PoW` into `ns-3-dev/src/application/model`

  2. Change the ./ns-3-dev/src/application/CMakeLists.txt file
     
  Add the following to the SOURCE_FILES list:

    model/pow.cc
    
  Add the following to the HEADER_FILES list:

    model/pow.h
  
  ### 5. Run Simulator
    ./ns-3-dev/ns3 clean
    ./ns-3-dev/ns3 configure
    ./ns-3-dev/ns3 build
    ./ns-3-dev/ns3 run blockchain-simulator
     
  ### 6. Change algorithm
  If you want to change the algorithm, you should follow `Consensus Algorithm` part with file in `PQC_algorithms` you want.
  And return the file to its original location. 

  Also change the file as follow:
  1. blockchain-simulator.cc
  2. Network-helper.cc
     
  What need to change in 1, 2 is indicated in the file.
  
  Then, follow `Run Simulator` part
  
## Reference
  ### [Blockchain Simulator & Network Helper](https://github.com/zhayujie/blockchain-simulator)
  ### [OpenSSL](https://askubuntu.com/questions/1462945/make-j-failing-for-oqs-openssl)
  ### [liboqs](https://github.com/open-quantum-safe/liboqs)

## Contributor
## License

