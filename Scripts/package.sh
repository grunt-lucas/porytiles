#!/usr/bin/env bash

set -eu

# FIXME : build version and date not correctly passed, need to find idiomatic CMake way to handle this

package_release() {
  cp build/Porytiles-0.x/cli/porytiles "$output_directory/porytiles-$mode"
  cp build/Porytiles-0.x/tests/Porytiles0xTestSuite "$output_directory/porytiles-$mode"
  cp CHANGELOG.md "$output_directory/porytiles-$mode"
  cp README.md "$output_directory/porytiles-$mode"
  cp LICENSE "$output_directory/porytiles-$mode"
  mkdir -p "$output_directory/porytiles-$mode/Resources"
  cp -r Resources/Examples "$output_directory/porytiles-$mode/Resources"
  cp -r Resources/Tests "$output_directory/porytiles-$mode/Resources"
  cp -r Porytiles-0.x/vendor "$output_directory/porytiles-$mode"

  # Run the tests, bail package script if they fail
  pushd "$output_directory/porytiles-$mode"
  ./Porytiles0xTestSuite
  popd

  # Everything OK, make the zip
  zip -r "$output_directory/porytiles-$mode.zip" "$output_directory/porytiles-$mode"
}

linux_amd64_clang() {
  echo "Packaging release linux-amd64-clang..."
  mkdir -p "$output_directory/porytiles-$mode"
  export CXX="clang++"
  cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_FIND_FRAMEWORK=NEVER -B build
  pushd build
  cmake --build .
  popd
  package_release
}

linux_arm64_clang() {
  echo "Packaging release linux-arm64-clang..."
  mkdir -p "$output_directory/porytiles-$mode"
  export CXX="clang++"
  cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_FIND_FRAMEWORK=NEVER -B build
  pushd build
  cmake --build .
  popd
  package_release
}

macos_amd64_clang() {
  echo "Packaging release macos-amd64-clang..."
  mkdir -p "$output_directory/porytiles-$mode"
  export CXX="/usr/local/opt/llvm/bin/clang++"
  cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_FIND_FRAMEWORK=NEVER -B build
  pushd build
  cmake --build .
  popd
  package_release
}

macos_arm64_clang() {
  echo "Packaging release macos-arm64-clang..."
  mkdir -p "$output_directory/porytiles-$mode"
  export CXX="/opt/homebrew/opt/llvm/bin/clang++"
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
    linux-amd64-clang)
    linux_amd64_clang
    ;;

    linux-arm64-clang)
    linux_arm64_clang
    ;;

    macos-amd64-clang)
    macos_amd64_clang
    ;;

    macos-arm64-clang)
    macos_arm64_clang
    ;;

    *)
    echo "unknown mode: $mode"
    echo ""
    echo "Valid modes are:"
    echo "    linux-amd64-clang"
    echo "    linux-arm64-clang"
    echo "    macos-amd64-clang"
    echo "    macos-arm64-clang"
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
