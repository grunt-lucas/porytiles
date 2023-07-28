#!/usr/bin/env bash

set -Eeuo pipefail

usage() {
    cat <<EOF
Usage: coverage.sh <show> <file ...>
       coverage.sh <report> <file ...>
       coverage.sh --help

Display coverage or coverage report for one or more source files. To change
the path to the LLVM coverage tools, set environment variable \`LLVM_COV_PATH'.

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
    [[ ${#args[@]} -lt 2 ]] && usage_exit_error

    return 0
}

llvm-profdata-wrapper() {
    if [[ ! -z "${LLVM_COV_PATH}" ]]; then
        "${LLVM_COV_PATH}"/llvm-profdata "$@"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        xcrun llvm-profdata "$@"
    else
        llvm-profdata "$@"
    fi
}

llvm-cov-wrapper() {
    if [[ ! -z "${LLVM_COV_PATH}" ]]; then
        "${LLVM_COV_PATH}"/llvm-cov "$@"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        xcrun llvm-cov "$@"
    else
        llvm-cov "$@"
    fi
}

parse_params "$@"

if [[ ! -f ./debug/bin/porytiles-tests ]]; then
    echo "error: debug test harness \`debug/bin/porytiles-tests' not found"
    echo "Please run \`make debug-check' to build the test harness and generate profile data."
    echo
    echo "Note: Porytiles uses the LLVM coverage tools for coverage, so it requires that you build"
    echo "with Clang to generate profile data. Use the \`CXX' variable to set your compiler to Clang."
    exit 1
fi

if [[ ! -f default.profraw ]]; then
    echo "error: profile data \`default.profraw' not found"
    echo "Please run \`make debug-check' to build the test harness and generate profile data."
    echo
    echo "Note: Porytiles uses the LLVM coverage tools for coverage, so it requires that you build"
    echo "with Clang to generate profile data. Use the \`CXX' variable to set your compiler to Clang."
    exit 1
fi

if [[ ${args[0]} == "show" ]]; then
    llvm-profdata-wrapper merge -o testcov.profdata default.profraw
    llvm-cov-wrapper show ./debug/bin/porytiles-tests -instr-profile=testcov.profdata "${args[@]:1}"
elif [[ ${args[0]} == "report" ]]; then
    llvm-profdata-wrapper merge -o testcov.profdata default.profraw
    llvm-cov-wrapper report ./debug/bin/porytiles-tests -instr-profile=testcov.profdata "${args[@]:1}"
else
    echo "error: unknown command \`${args[0]}'"
    usage_exit_error
fi
