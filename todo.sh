#!/usr/bin/env bash

set -Eeo pipefail

usage() {
    cat <<EOF
Usage: todo.sh all
       todo.sh TODO
       todo.sh FIXME
       todo.sh NOTE
       todo.sh FEATURE
       todo.sh --help

Display TODO related note comments.

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

if [[ ${args[0]} == "all" ]]; then
    rg 'TODO :'
    rg 'FIXME :'
    rg 'NOTE :'
    rg 'FEATURE :'
elif [[ ${args[0]} == "TODO" ]]; then
    rg 'TODO :'
elif [[ ${args[0]} == "FIXME" ]]; then
    rg 'FIXME :'
elif [[ ${args[0]} == "NOTE" ]]; then
    rg 'NOTE :'
elif [[ ${args[0]} == "FEATURE" ]]; then
    rg 'FEATURE :'
else
    echo "error: unknown command \`${args[0]}'"
    usage_exit_error
fi
