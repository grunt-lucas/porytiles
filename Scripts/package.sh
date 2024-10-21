#!/usr/bin/env bash

set -eu

# FIXME : build version and date not correctly passed, need to find idiomatic CMake way to handle this

package_release() {
  cp build/Porytiles-1.x/cli/porytiles "$output_directory/porytiles-$mode"
  cp build/Porytiles-1.x/tests/Porytiles1xTestSuite "$output_directory/porytiles-$mode"
  cp CHANGELOG.md "$output_directory/porytiles-$mode"
  cp README.md "$output_directory/porytiles-$mode"
  cp LICENSE "$output_directory/porytiles-$mode"
  mkdir -p "$output_directory/porytiles-$mode/Resources"
  cp -r Resources/Examples "$output_directory/porytiles-$mode/Resources"
  cp -r Resources/Tests "$output_directory/porytiles-$mode/Resources"
  cp -r Porytiles-1.x/vendor "$output_directory/porytiles-$mode"

  # Run the tests, bail package script if they fail
  pushd "$output_directory/porytiles-$mode"
  ./Porytiles1xTestSuite
  popd

  # Everything OK, make the zip
  zip -r "$output_directory/porytiles-$mode.zip" "$output_directory/porytiles-$mode"
}

macos_arm64() {
  echo "Packaging release macos-arm64..."
  mkdir -p "$output_directory/porytiles-$mode"
  export CXX="/opt/homebrew/opt/llvm@16/bin/clang++"
  cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_FIND_FRAMEWORK=NEVER -B build
  pushd build
  cmake --build .
  popd
  package_release
}

linux_aarch64() {
  echo "Packaging release linux-aarch64..."
  mkdir -p "$output_directory/porytiles-$mode"
  export CXX="clang++-16"
  cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_FIND_FRAMEWORK=NEVER -B build
  pushd build
  cmake --build .
  popd
  package_release
}

linux_amd64() {
  echo "Packaging release linux-amd64..."
  mkdir -p "$output_directory/porytiles-$mode"
  export CXX="clang++-16"
  cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_FIND_FRAMEWORK=NEVER -B build
  pushd build
  cmake --build .
  popd
  package_release
}

main() {
  if [[ ! -d "$output_directory" ]]
  then
    echo "$output_directory: not a directory"
    exit 1
  fi

  case $mode in
    macos-arm64)
    macos_arm64
    ;;

    linux-aarch64)
    linux_aarch64
    ;;

    linux-amd64)
    linux_amd64
    ;;

    *)
    echo "unknown mode: $mode"
    echo ""
    echo "Valid modes are:"
    echo "    macos-arm64"
    echo "    linux-aarch64"
    echo "    linux-amd64"
    exit 1
    ;;
  esac
}

if [[ ! -f .porytiles-marker-file ]]
then
    echo "Script must run in main Porytiles directory"
    exit 1
fi

if [[ "$#" != 3 ]]
then
  echo "invalid arguments: usage: ./package.sh <mode> <build-version> <output-directory>"
  exit 1
fi

mode=$1
porytiles_build_version=$2
output_directory=$3
porytiles_build_date=$(date -uIseconds)
main
