#!/bin/bash

RED='\033[0;31m'
GREEN='\033[1;32m'
BLUE='\033[0;37m'
NC='\033[0m'

set -e

ROOT="$(git rev-parse --show-toplevel)"
SPEC="${ROOT}/opengigabyte-driver-dkms.spec"

# Keep the tarball version in step with dkms.conf, which is what actually
# decides the /usr/src/opengigabyte-driver-<ver> path the spec packages.
VERSION=$(grep -oP '(?<=^PACKAGE_VERSION=")[^"]+' "${ROOT}/install_files/dkms/dkms.conf")
SPEC_VERSION=$(grep -oP '(?<=^%global dkms_version )\S+' "${SPEC}")

if [ "${VERSION}" != "${SPEC_VERSION}" ]; then
    echo -e "${RED}Version mismatch: dkms.conf has ${VERSION}, spec has ${SPEC_VERSION}${NC}"
    exit 1
fi

TOPDIR=$(rpm -E '%{_topdir}')
mkdir -p "${TOPDIR}"/{SOURCES,SPECS,BUILD,BUILDROOT,RPMS,SRPMS}

echo -e "${BLUE}Building opengigabyte ${VERSION}${NC}"

# Archive the working tree (not just HEAD) so uncommitted driver changes are
# picked up, matching the behaviour of build_debs.sh.
TARBALL="${TOPDIR}/SOURCES/opengigabyte-${VERSION}.tar.gz"
tar --exclude-vcs --exclude-vcs-ignores \
    --exclude=dist --exclude='*.tar.gz' \
    --transform "s,^\.,opengigabyte-${VERSION}," \
    -zcf "${TARBALL}" -C "${ROOT}" .

echo -e "${BLUE}Source tarball: ${TARBALL}${NC}"

cp -f "${SPEC}" "${TOPDIR}/SPECS/"

echo -e "${BLUE}Running rpmbuild${NC}\n"
if rpmbuild -bb "${TOPDIR}/SPECS/$(basename "${SPEC}")"; then
    mkdir -p "${ROOT}/dist"
    find "${TOPDIR}/RPMS" -name "opengigabyte-driver-dkms-${VERSION}*.rpm" \
        -exec cp -v {} "${ROOT}/dist/" \;
    echo -e "\n${GREEN}RPM built successfully. Copied to ./dist/${NC}"
    echo -e "${GREEN}Install with: sudo dnf install ./dist/opengigabyte-driver-dkms-${VERSION}*.rpm${NC}"
else
    echo -e "${RED}rpmbuild failed${NC}"
    exit 1
fi
