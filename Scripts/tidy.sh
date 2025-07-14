#!/usr/bin/env bash

set -Eeuox pipefail
shopt -s globstar

usage() {
    cat <<EOF
Usage: tidy.sh
       tidy.sh <file ...>
       tidy.sh --help

Run 'clang-tidy' on the provided source files. If no source files are
provided, runs on the default set of source files (the Porytiles2 project).

Use TIDY_PATH env var to specify a path to your 'clang-tidy' executable.

Options:
    -h, --help      Print this help and exit.

EOF
}

usage_exit_ok() {
    usage
    exit
}

usage_exit_error() {
    usage
    exit 1
}

parse_params() {
    while :; do
        case "${1-}" in
            -h | --help) usage_exit_ok ;;
            -?*) die "Unknown option: $1" ;;
            *) break ;;
        esac
        shift
    done

    args=("$@")

    return 0
}

if [[ ! -f .porytiles-marker-file ]]
then
    echo "Script must run in main Porytiles directory"
    exit 1
fi

parse_params "$@"
if [[ ${#args[@]} -lt 1 ]]; then
    ${TIDY_PATH:-clang-tidy} \
    -checks='cert-*' \
    -header-filter='.*' \
    Porytiles2/**/*.{h,hpp,cpp} \
    -- \
    --std=c++23 \
    -IPorytiles2/include \
    -Ibuild/_deps/fmt-src/include \
    -Ibuild/_deps/cli11_proj-src/include \
    -Ibuild/_deps/gsl-src/include \
    -Ibuild/_deps/cimg-src \
    -Ibuild/_deps/googletest-src/googletest/include \
    "$(pkg-config --cflags libpng)"
else
    ${TIDY_PATH:-clang-tidy} \
    -checks='cert-*' \
    -header-filter='.*' \
    "${args[@]}" \
    -- \
    --std=c++23 \
    -Iinclude \
    "$(pkg-config --cflags libpng)" \
    -Idoctest-2.4.11 \
    -Ipng++-0.2.9 \
    -Ifmt-10.0.0/include
fi


