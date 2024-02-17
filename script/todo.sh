#!/usr/bin/env bash

set -Eeo pipefail

usage() {
    cat <<EOF
Usage: todo.sh all
       todo.sh {TODO,todo}
       todo.sh {FIXME,fixme}
       todo.sh {NOTE,note}
       todo.sh {FEATURE,feature}
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

if [[ ! -f .porytiles-marker-file ]]
then
    echo "Script must run in main Porytiles directory"
    exit 1
fi

parse_params "$@"

if [[ ${args[0]} == "all" ]]; then
    rg 'TODO :'
    rg 'FIXME :'
    rg 'NOTE :'
    rg 'FEATURE :'
elif [[ ${args[0]} == "TODO" || ${args[0]} == "todo" ]]; then
    rg 'TODO :'
elif [[ ${args[0]} == "FIXME" || ${args[0]} == "fixme" ]]; then
    rg 'FIXME :'
elif [[ ${args[0]} == "NOTE" || ${args[0]} == "note" ]]; then
    rg 'NOTE :'
elif [[ ${args[0]} == "FEATURE" || ${args[0]} == "feature" ]]; then
    rg 'FEATURE :'
else
    echo "error: unknown command \`${args[0]}'"
    usage_exit_error
fi
