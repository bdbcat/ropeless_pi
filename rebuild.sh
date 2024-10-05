rm -rf build
mkdir build && cd build
cmake -DOCPN_TARGET_TUPLE='ubuntu-wx32;22.04;x86_64' ..
make tarball