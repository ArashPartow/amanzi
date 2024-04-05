#!/bin/bash

AMANZI_BRANCH=master
AMANZI_SOURCE_DIR=/ascem/amanzi/repos/amanzi-master
AMANZI_TPLS_VER=0.98.0

# architecture options - if TRUE, build for arm and x86_64,
# if FALSE, build only for current system architecture.
MULTIARCH=FALSE

# output style - change to "--progress=plain" for debugging
OUTPUT_STYLE=""

LANL_PROXY="--build-arg http_proxy=proxyout.lanl.gov:8080 --build-arg https_proxy=proxyout.lanl.gov:8080"

AMANZI_GIT_LATEST_TAG_VER=`(cd $AMANZI_SOURCE_DIR; git tag -l amanzi-* | tail -n1 | sed -e 's/amanzi-//')`
AMANZI_GIT_GLOBAL_HASH=`(cd $AMANZI_SOURCE_DIR; git rev-parse --short HEAD)`
AMANZI_VER="${AMANZI_GIT_LATEST_TAG_VER}_${AMANZI_GIT_GLOBAL_HASH}"

echo ""
echo "AMANZI_SOURCE_DIR = $AMANZI_SOURCE_DIR"
echo " - latest tag       $AMANZI_GIT_LATEST_TAG_VER"
echo " - global hash      $AMANZI_GIT_GLOBAL_HASH"
echo " - version string   $AMANZI_VER"
echo ""

if $MULTIARCH
then
    docker buildx build --platform=linux/amd64,linux/arm64 --no-cache \
        --build-arg amanzi_branch=${AMANZI_BRANCH} \
        --build-arg amanzi_tpls_ver=${AMANZI_TPLS_VER} ${LANL_PROXY} \
        $OUTPUT_STYLE \
        -f ${AMANZI_SOURCE_DIR}/Docker/Dockerfile-Amanzi \
        -t metsi/amanzi:${AMANZI_VER} .
else
    docker build --no-cache \
        --build-arg amanzi_branch=${AMANZI_BRANCH} \
        --build-arg amanzi_tpls_ver=${AMANZI_TPLS_VER} ${LANL_PROXY} \
        $OUTPUT_STYLE \
        -f ${AMANZI_SOURCE_DIR}/Docker/Dockerfile-Amanzi \
        -t metsi/amanzi:${AMANZI_VER} .
fi

docker tag metsi/amanzi:${AMANZI_VER} metsi/amanzi:latest

