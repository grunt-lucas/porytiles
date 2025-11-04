#!/usr/bin/env bash

set -Eeuox pipefail
shopt -s globstar

usage() {
    cat <<EOF
Usage: format.sh
       format.sh <file ...>
       format.sh --help

Run 'clang-format' on the provided source files. If no source files are
provided, runs on the default set of source files (the Porytiles2 project).

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

if [[ ! -f .porytiles-marker-file ]]; then
    echo "Script must run in main Porytiles directory"
    exit 1
fi

parse_params "$@"
if [[ ${#args[@]} -lt 1 ]]; then
    clang-format -style=file -i Porytiles2/**/*.{h,hpp,cpp,ipp} # \
        # Porytiles1/include/**/*.{h,hpp,cpp} \
        # Porytiles1/lib/**/*.{h,hpp,cpp} \
        # Porytiles1/tests/**/*.{h,hpp,cpp} \
        # Porytiles1/tools/**/*.{h,hpp,cpp}
else
    clang-format -style=file -i "${args[@]}"
fi
