#!/usr/bin/env bash

set -eu

macos_aarch64() {
  echo "Packaging release macos-aarch64..."
  mkdir -p "$output_directory/porytiles-$mode"
  make clean
  CXXFLAGS="-nostdinc++ -nostdlib++ -isystem /opt/homebrew/opt/llvm@16/include/c++/v1" \
  LDFLAGS="-L /opt/homebrew/opt/llvm@16/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm@16/lib/c++ -lc++ -lc++abi" \
  CXX=/opt/homebrew/opt/llvm/bin/clang++ \
  make release-check
  cp release/bin/porytiles "$output_directory/porytiles-$mode"
  cp CHANGELOG.md "$output_directory/porytiles-$mode"
  cp README.md "$output_directory/porytiles-$mode"
  cp LICENSE "$output_directory/porytiles-$mode"
  cp -r res "$output_directory/porytiles-$mode"
  cp -r libs "$output_directory/porytiles-$mode"
  zip -r "$output_directory/porytiles-$mode.zip" "$output_directory/porytiles-$mode"
}

macos_amd64() {
  echo "Packaging release macos-amd64..."
  mkdir -p "$output_directory/porytiles-$mode"
  make clean
  CXXFLAGS="-nostdinc++ -nostdlib++ -isystem /usr/local/opt/llvm@16/include/c++/v1" \
  LDFLAGS="-L /usr/local/opt/llvm@16/lib/c++ -Wl,-rpath,/usr/local/opt/llvm@16/lib/c++ -lc++ -lc++abi" \
  CXX=/usr/local/opt/llvm/bin/clang++ \
  make release-check
  cp release/bin/porytiles "$output_directory/porytiles-$mode"
  cp CHANGELOG.md "$output_directory/porytiles-$mode"
  cp README.md "$output_directory/porytiles-$mode"
  cp LICENSE "$output_directory/porytiles-$mode"
  cp -r res "$output_directory/porytiles-$mode"
  cp -r libs "$output_directory/porytiles-$mode"
  zip -r "$output_directory/porytiles-$mode.zip" "$output_directory/porytiles-$mode"
}

main() {
  if [[ ! -f .porytiles-marker-file ]]
  then
    echo "Package script must run in main Porytiles directory"
    exit 1
  fi

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
    macos-amd64
    ;;

    *)
    echo "unknown mode: $mode"
    exit 1
    ;;
  esac
}

mode=$1
output_directory=$2
main
