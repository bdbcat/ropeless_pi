rem
rem  Upload the .tar.gz and .xml artifacts to cloudsmith
rem

echo OFF
if "%CLOUDSMITH_API_KEY%" == "" (
    echo Warning: CLOUDSMITH_API_KEY is not available, giving up.
    echo Metadata: ropeless-2.1-ubuntu-wx32-22.04.xml
    echo Tarball: ropeless-2.1.0.0+2403281537.36e8a64_ubuntu-wx32-22.04-x86_64.tar.gz
    echo Version: 2.1.0.0+2403281537.36e8a64
    exit /b 0
)
echo Using CLOUDSMITH_API_KEY: %CLOUDSMITH_API_KEY:~,4%%...
echo ON

cloudsmith push raw --no-wait-for-sync ^
    --name ropeless-2.1-ubuntu-wx32-22.04-metadata ^
    --version 2.1.0.0+2403281537.36e8a64 ^
    --summary "Plugin metadata for automatic installation" ^
    --republish ^
    david-register/ocpn-plugins-unstable ropeless-2.1-ubuntu-wx32-22.04.xml

cloudsmith push raw --no-wait-for-sync ^
    --name ropeless-2.1-ubuntu-wx32-22.04-tarball ^
    --version 2.1.0.0+2403281537.36e8a64 ^
    --summary "Plugin tarball for automatic installation" ^
    --republish ^
    david-register/ocpn-plugins-unstable ropeless-2.1.0.0+2403281537.36e8a64_ubuntu-wx32-22.04-x86_64.tar.gz
