
  execute_process(
    COMMAND  cmake -E sha256sum /home/colin/ropeless_pi/build/ropeless-2.1.0.0+2403281537.36e8a64_ubuntu-wx32-22.04-x86_64.tar.gz
    OUTPUT_FILE /home/colin/ropeless_pi/build/ropeless-2.1.0.0+2403281537.36e8a64_ubuntu-wx32-22.04-x86_64.sha256
  )
  file(READ /home/colin/ropeless_pi/build/ropeless-2.1.0.0+2403281537.36e8a64_ubuntu-wx32-22.04-x86_64.sha256 _SHA256)
  string(REGEX MATCH "^[^ ]*" checksum ${_SHA256} )
  configure_file(
    /home/colin/ropeless_pi/build/ropeless-2.1-ubuntu-wx32-22.04.xml.in
    /home/colin/ropeless_pi/build/ropeless-2.1-ubuntu-wx32-22.04.xml
    @ONLY
  )
