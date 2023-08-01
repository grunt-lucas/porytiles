#!/usr/bin/env bash

set -Eeuo pipefail

usage() {
    cat <<EOF
Usage: format.sh <file ...>
       format.sh --help

Run \`clang-format' on the provided source files.

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
clang-format -style=file -i "${args[@]}"
