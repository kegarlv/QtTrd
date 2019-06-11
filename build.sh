#!/bin/sh

#Install dependencies
brew install qt5
brew install openssl

#Build the project
cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt/5.12.3/ \
         -DWITH_QT5=1 \
         -DCMAKE_BUILD_TYPE=RELEASE \
         -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
         -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib/ \
         -DCMAKE_LIBRARY_PATH=/usr/local/opt/openssl/lib

LIBRARY_PATH="/usr/local/opt/openssl/lib/:"$LIBRARY_PATH make
