#!/usr/bin/env bash

#
# Upload the .tar.gz and .xml artifacts to cloudsmith
#

if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Warning: $CLOUDSMITH_API_KEY is not available, giving up.'
    echo 'Metadata: ropeless-0.4-ubuntu-wx32-22.04.xml'
    echo 'Tarball: ropeless-0.4.0.0+2403262323.17bf0fe_ubuntu-wx32-22.04-x86_64.tar.gz'
    echo 'Version: 0.4.0.0+2403262323.17bf0fe'
    exit 0
fi

echo "Using CLOUDSMITH_API_KEY: ${CLOUDSMITH_API_KEY:0:4}..."

if [ -f ~/.uploadrc ]; then source ~/.uploadrc; fi
set -xe

cloudsmith push raw --no-wait-for-sync \
    --name ropeless-0.4-ubuntu-wx32-22.04-metadata \
    --version 0.4.0.0+2403262323.17bf0fe \
    --summary "Plugin metadata for automatic installation" \
    --republish \
    david-register/ocpn-plugins-unstable ropeless-0.4-ubuntu-wx32-22.04.xml

cloudsmith push raw --no-wait-for-sync \
    --name ropeless-0.4-ubuntu-wx32-22.04-tarball \
    --version 0.4.0.0+2403262323.17bf0fe \
    --summary "Plugin tarball for automatic installation" \
    --republish \
    david-register/ocpn-plugins-unstable ropeless-0.4.0.0+2403262323.17bf0fe_ubuntu-wx32-22.04-x86_64.tar.gz
