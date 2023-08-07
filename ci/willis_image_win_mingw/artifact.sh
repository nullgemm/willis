#!/bin/bash

tag=$(git tag --sort v:refname | tail -n 1)
release=willis_bin_"$tag"

docker cp willis_container_win_mingw:/willis/"$release" .
