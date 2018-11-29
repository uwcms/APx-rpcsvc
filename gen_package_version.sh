#!/bin/bash

cat <<EOF
This directory contains an rpcsvc developer package.

These files are a part of the rpcsvc project, may change in future updates, and most should not be changed locally.

--- RPCSVC VERSION INFORMATION ---
EOF

git rev-parse HEAD
git status -s .
echo

echo ---- PACKAGE FILE LIST ---
(cd "$1"; find -type f ! -path '*/.git/*' | sort)
