#!/usr/bin/env bash

set -eu

package_release() {
  cp release/bin/porytiles "$output_directory/porytiles-$mode"
  cp CHANGELOG.md "$output_directory/porytiles-$mode"
  cp README.md "$output_directory/porytiles-$mode"
  cp LICENSE "$output_directory/porytiles-$mode"
  cp -r res "$output_directory/porytiles-$mode"
  cp -r libs "$output_directory/porytiles-$mode"
  zip -r "$output_directory/porytiles-$mode.zip" "$output_directory/porytiles-$mode"
}

macos_aarch64() {
  echo "Packaging release macos-aarch64..."
  mkdir -p "$output_directory/porytiles-$mode"
  make clean
  export CXX=/opt/homebrew/opt/llvm@16/bin/clang++
  export CXXFLAGS="-nostdinc++ -nostdlib++ -isystem /opt/homebrew/opt/llvm@16/include/c++/v1 -DPORYTILES_BUILD_VERSION_=${porytiles_build_version} -DPORYTILES_BUILD_DATE_=${porytiles_build_date}"
  export LDFLAGS="-L /opt/homebrew/opt/llvm@16/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm@16/lib/c++"
  make release-check
  package_release
}

# Homebrew on GitHub AMD64 MacOS runners seems to have moved to /opt/homebrew?
macos_amd64() {
  echo "Packaging release macos-amd64..."
  mkdir -p "$output_directory/porytiles-$mode"
  make clean
  export CXX=/opt/homebrew/opt/llvm@16/bin/clang++
  export CXXFLAGS="-nostdinc++ -nostdlib++ -isystem /opt/homebrew/opt/llvm@16/include/c++/v1 -DPORYTILES_BUILD_VERSION_=${porytiles_build_version} -DPORYTILES_BUILD_DATE_=${porytiles_build_date}"
  export LDFLAGS="-L /opt/homebrew/opt/llvm@16/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm@16/lib/c++"
  make release-check
  package_release
}

linux_aarch64() {
  echo "Packaging release linux-aarch64..."
  mkdir -p "$output_directory/porytiles-$mode"
  make clean
  export CXX=clang++-16
  export CXXFLAGS="-stdlib=libc++ -DPORYTILES_BUILD_VERSION_=${porytiles_build_version} -DPORYTILES_BUILD_DATE_=${porytiles_build_date}"
  export LDFLAGS="-stdlib=libc++ -static"
  make release-check
  package_release
}

linux_amd64() {
  echo "Packaging release linux-amd64..."
  mkdir -p "$output_directory/porytiles-$mode"
  make clean
  export CXX=clang++-16
  export CXXFLAGS="-stdlib=libc++ -DPORYTILES_BUILD_VERSION_=${porytiles_build_version} -DPORYTILES_BUILD_DATE_=${porytiles_build_date}"
  export LDFLAGS="-stdlib=libc++ -static"
  make release-check
  package_release
}

main() {
  if [[ ! -d "$output_directory" ]]
  then
    echo "$output_directory: not a directory"
    exit 1
  fi

  case $mode in
    macos-aarch64)
    macos_aarch64
    ;;

    macos-amd64)
    macos_amd64
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
    echo "    macos-aarch64"
    echo "    macos-amd64"
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
