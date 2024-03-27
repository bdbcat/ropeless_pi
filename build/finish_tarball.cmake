
    execute_process(
      COMMAND cmake -E rm -rf app/ropeless-0.4-ubuntu-wx32-22.04
    )
    execute_process(
      COMMAND cmake -E rename app/files app/ropeless-0.4-ubuntu-wx32-22.04
    )
    execute_process(
      WORKING_DIRECTORY /home/colin/ropeless_pi/build/app
      COMMAND
        cmake -E
        tar -czf ../ropeless-0.4.0.0+2403272137.829f5d9_ubuntu-wx32-22.04-x86_64.tar.gz --format=gnutar ropeless-0.4-ubuntu-wx32-22.04
    )
    message(STATUS "Creating tarball ropeless-0.4.0.0+2403272137.829f5d9_ubuntu-wx32-22.04-x86_64.tar.gz")

    execute_process(COMMAND cmake -P /home/colin/ropeless_pi/build/checksum.cmake)
    message(STATUS "Computing checksum in ropeless-0.4-ubuntu-wx32-22.04.xml")
  