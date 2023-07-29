## INSTALL: Building Plugins generic README.

Install build dependencies as described in the 
[manual](https://opencpn-manuals.github.io/main/AlternativeWorkflow/Local-Build.html)
Then clone this repository, enter it and make
`rm -rf build; mkdir build; cd build`.

A "normal" (not flatpak) tar.gz tarball which can be used by the new plugin
installer available from OpenCPN 5.2.0 is built using:

    $ cmake ..
    $ make tarball

To build the flatpak tarball:

    $ cmake ..
    $ make flatpak

Historically, it has been possible to build legacy packages like
an NSIS installer on Windows and .deb packages on Linux. This ability
has been removed in the 5.6.0 cycle.

#### Building for Android

Builds for android requires an ndk installation and an updated cmake,
see manual (above).

To build an android aarch64 tarball:

    $ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/android-aarch64-toolchain.cmake ..
    $ make

To build an android armhf tarball

    $ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/android-armhf-toolchain.cmake ..
    $ make

#### Building on windows (MSVC)
On windows, a different workflow is used:

    > ..\buildwin\win_deps.bat
    > cmake -T v141_xp -G "Visual Studio 15 2017" ^
           -DCMAKE_BUILD_TYPE=RelWithDebInfo  ..
    > cmake --build . --target tarball --config RelWithDebInfo


#### Android plugin import/installation:

    1.  Get and install from Android Playstore the free file manager application called XPlore.
    2.  Connect Android device to PC by USB cable.  Verify that PC has access to device.
    3.  From PC, copy the attached tarball file to the device, at location like "Internal shared storage/Android/data/org.opencpn.opencpn/files"
    4.  Start OpenCPN on Android.  Exit OpenCPN.  This step validates OpenCPN configuration files.
    5.  Using XPlore, find and edit the file ".../Android/data/org.opencpn.opencpn/files/opencpn.conf"
    6.  Add the following to section [Plugins] (on next line):  CatalogExpert=1
    7.  Save and close XPlore.
    8.  Start OpenCPN.
    9.  Go to settings(gear icon)->Plugins->Import Plugin.  Navigate to the tarball file copied in step 3.  Select it.  Follow the supplied prompts.
    10. Should be all set to go.  The new plugin icon should appear in the main OpenCPN toolbar.


