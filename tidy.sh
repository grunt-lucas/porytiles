#!/usr/bin/env bash

set -Eeuo pipefail

usage() {
    cat <<EOF
Usage: tidy.sh <file ...>
       tidy.sh --help

Run \`clang-tidy' on the provided source files.

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
    [[ ${#args[@]} -lt 1 ]] && usage_exit_error

    return 0
}

parse_params "$@"
clang-tidy \
    -checks='cert-*' \
    -header-filter='.*' \
    "${args[@]}" \
    -- \
    --std=c++20 \
    -Iinclude \
    $(pkg-config --cflags libpng) \
    -Idoctest-2.4.11 \
    -Ipng++-0.2.9 \
    -Ifmt-10.0.0/include
