#!/bin/bash

DESCR=$(git describe --tags)
DESCR_EXACT=$(git describe --tags --exact-match 2>/dev/null)

if [[ "${DESCR}" == "${DESCR_EXACT}" ]]; then
  # tagged release
  VERSION_STRING="${DESCR}"
else
  # development (untagged) revision
  VERSION_STRING="BETA ${DESCR}"
fi

echo -n ${VERSION_STRING}
