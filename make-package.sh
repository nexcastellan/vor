#!/bin/bash -e

REVISION="$(svn info . | grep '^Revision: ' | sed -e 's/[^0-9]//g')"
PACKAGE="usersearchd-r${REVISION}"

echo "Building source package '${PACKAGE}.tar.gz'."
svn export . "${PACKAGE}"
tar cvfz "${PACKAGE}.tar.gz" "${PACKAGE}"
rm -rfv "${PACKAGE}"
echo "Completed build of source package '${PACKAGE}.tar.gz'."
