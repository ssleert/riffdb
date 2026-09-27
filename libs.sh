mkdir -p libs
git clone --recursive --depth=1 https://github.com/microsoft/mimalloc.git libs/mimalloc & 
git clone --recursive --depth=1 https://github.com/wolfSSL/wolfssl.git libs/wolfssl &

wait
