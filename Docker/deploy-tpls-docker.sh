#!/bin/bash

# MPI installed in the Docker image
# Options: openmpi, mpich
MPI_DISTRO=mpich
MPI_VERSION=4.0.3  # ignored if build_mpi = False
BUILD_MPI=True

PETSC_VER=3.20
TRILINOS_VER=14-2-fc55b9cd

AMANZI_BRANCH=master
AMANZI_SOURCE_DIR=/ascem/amanzi/repos/amanzi-master
AMANZI_TPLS_VER=0.98.8

# architecture options - if TRUE, build for arm and x86_64,
# if FALSE, build only for current system architecture.
MULTIARCH=FALSE

# output style - change to "--progress=plain" for debugging
OUTPUT_STYLE=""

LANL_PROXY="--build-arg http_proxy=proxyout.lanl.gov:8080 --build-arg https_proxy=proxyout.lanl.gov:8080"

if $MULTIARCH
then
    docker buildx build --platform=linux/amd64,linux/arm64 \
        --no-cache $OUTPUT_STYLE \
        --build-arg petsc_ver=${PETSC_VER} \
        --build-arg trilinos_ver=${TRILINOS_VER} \
        --build-arg amanzi_branch=${AMANZI_BRANCH} \
        --build-arg build_mpi=${BUILD_MPI} \
        --build-arg mpi_flavor=${MPI_DISTRO} \
        --build-arg mpi_version=${MPI_VERSION} \
        -f ${AMANZI_SOURCE_DIR}/Docker/Dockerfile-TPLs \
        -t metsi/amanzi-tpls:${AMANZI_TPLS_VER}-${MPI_DISTRO} .
else
    docker build --no-cache $OUTPUT_STYLE \
        --build-arg petsc_ver=${PETSC_VER} \
        --build-arg trilinos_ver=${TRILINOS_VER} \
        --build-arg amanzi_branch=${AMANZI_BRANCH} \
        --build-arg build_mpi=${BUILD_MPI} \
        --build-arg mpi_flavor=${MPI_DISTRO} \
        --build-arg mpi_version=${MPI_VERSION} \
        -f ${AMANZI_SOURCE_DIR}/Docker/Dockerfile-TPLs \
        -t metsi/amanzi-tpls:${AMANZI_TPLS_VER}-${MPI_DISTRO} .
fi

#docker build --no-cache --progress=plain --build-arg petsc_ver=${PETSC_VER} --build-arg trilinos_ver=${TRILINOS_VER} --build-arg amanzi_branch=${AMANZI_BRANCH} -f ${AMANZI_SOURCE_DIR}/Docker/Dockerfile-TPLs -t metsi/amanzi-tpls:${AMANZI_TPLS_VER}-${MPI_DISTRO} .
