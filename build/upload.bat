rem
rem  Upload the .tar.gz and .xml artifacts to cloudsmith
rem

echo OFF
if "%CLOUDSMITH_API_KEY%" == "" (
    echo Warning: CLOUDSMITH_API_KEY is not available, giving up.
    echo Metadata: ropeless-0.4-ubuntu-wx32-22.04.xml
    echo Tarball: ropeless-0.4.0.0+2403272137.829f5d9_ubuntu-wx32-22.04-x86_64.tar.gz
    echo Version: 0.4.0.0+2403272137.829f5d9
    exit /b 0
)
echo Using CLOUDSMITH_API_KEY: %CLOUDSMITH_API_KEY:~,4%%...
echo ON

cloudsmith push raw --no-wait-for-sync ^
    --name ropeless-0.4-ubuntu-wx32-22.04-metadata ^
    --version 0.4.0.0+2403272137.829f5d9 ^
    --summary "Plugin metadata for automatic installation" ^
    --republish ^
    david-register/ocpn-plugins-unstable ropeless-0.4-ubuntu-wx32-22.04.xml

cloudsmith push raw --no-wait-for-sync ^
    --name ropeless-0.4-ubuntu-wx32-22.04-tarball ^
    --version 0.4.0.0+2403272137.829f5d9 ^
    --summary "Plugin tarball for automatic installation" ^
    --republish ^
    david-register/ocpn-plugins-unstable ropeless-0.4.0.0+2403272137.829f5d9_ubuntu-wx32-22.04-x86_64.tar.gz
