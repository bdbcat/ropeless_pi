
    execute_process(
      COMMAND
        flatpak-builder --force-clean --keep-build-dirs
          /home/colin/ropeless_pi/build/app /home/colin/ropeless_pi/build/org.opencpn.OpenCPN.Plugin.ropeless.yaml
    )
    # Copy the data out of the sandbox to installation directory
    execute_process(
      WORKING_DIRECTORY /home/colin/ropeless_pi/build
      COMMAND
        flatpak-builder --run app /home/colin/ropeless_pi/build/org.opencpn.OpenCPN.Plugin.ropeless.yaml
           bash copy_out libropeless_pi.so /home/colin/ropeless_pi/build
    )
    if (NOT EXISTS app/files/lib/opencpn/libropeless_pi.so)
      message(FATAL_ERROR "Cannot find generated file libropeless_pi.so")
    endif ()
    execute_process(
      COMMAND bash -c "sed -e '/@checksum@/d'           < ropeless-0.4-ubuntu-wx32-22.04.xml.in > app/files/metadata.xml"
    )
    if (Debug MATCHES Release|MinSizeRel)
      message(STATUS "Stripping app/files/lib/opencpn/libropeless_pi.so")
      execute_process(
        COMMAND strip app/files/lib/opencpn/libropeless_pi.so
      )
    endif ()
    execute_process(
      WORKING_DIRECTORY /home/colin/ropeless_pi/build/app
      COMMAND mv -fT files ropeless-0.4-ubuntu-wx32-22.04
    )
    execute_process(
      WORKING_DIRECTORY  /home/colin/ropeless_pi/build/app
      COMMAND
        cmake -E
        tar -czf ../ropeless-0.4.0.0+2403271613.4b7d6a6_ubuntu-wx32-22.04-x86_64.tar.gz --format=gnutar ropeless-0.4-ubuntu-wx32-22.04
    )
    message(STATUS "Building ropeless-0.4.0.0+2403271613.4b7d6a6_ubuntu-wx32-22.04-x86_64.tar.gz")
    execute_process(
      COMMAND cmake -P /home/colin/ropeless_pi/build/checksum.cmake
    )
    message(STATUS "Computing checksum in ropeless-0.4-ubuntu-wx32-22.04.xml")
  