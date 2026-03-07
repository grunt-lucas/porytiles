#pragma once

#include <iostream>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles2/infra/cli/cli_completion_data.hpp"

#include "command.hpp"

/**
 * @brief Command that outputs shell completion scripts.
 *
 * @details
 * CompletionCommand generates shell completion scripts for bash, zsh, and fish. It uses the
 * generated CLI option metadata to provide completions for all config options, including
 * enum choices.
 *
 * The generated scripts also support dynamic tileset name completion by calling the
 * `list-tilesets` subcommand. This provides context-sensitive filtering:
 * - compile-tileset: shows only managed tilesets
 * - decompile-tileset: shows only managed tilesets
 * - import-tileset: shows only unmanaged tilesets
 * - create-tileset: no completion (user provides new name)
 */
class CompletionCommand final : public Command {
  public:
    explicit CompletionCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<shell>", shell_, "Shell type: bash, zsh, or fish")->required();
    }

    void Run() override
    {
        if (shell_ == "bash") {
            output_bash_completion();
        }
        else if (shell_ == "zsh") {
            output_zsh_completion();
        }
        else if (shell_ == "fish") {
            output_fish_completion();
        }
        else {
            std::cerr << "Unknown shell type: " << shell_ << "\n";
            std::cerr << "Supported shells: bash, zsh, fish\n";
            throw CLI::RuntimeError{1};
        }
    }

  private:
    static constexpr auto kCommandName = "completion";
    static constexpr auto kCommandDesc = "Generate shell completion scripts.";
    static constexpr auto kCommandGroup = "UTILITIES";
    std::string shell_;

    void output_bash_completion()
    {
        std::cout << "# Bash completion for porytiles2\n";
        std::cout << "# Add to ~/.bashrc or ~/.bash_completion:\n";
        std::cout << "#   eval \"$(porytiles2 completion bash)\"\n";
        std::cout << "\n";

        // Helper function to extract project root from command line
        std::cout << "# Helper function to extract --project-root or -C from command line\n";
        std::cout << "_porytiles2_get_project_root() {\n";
        std::cout << "    local i\n";
        std::cout << "    for ((i=1; i < ${#COMP_WORDS[@]}; i++)); do\n";
        std::cout << "        case \"${COMP_WORDS[i]}\" in\n";
        std::cout << "            -C|--project-root)\n";
        std::cout << "                if [[ $((i+1)) -lt ${#COMP_WORDS[@]} ]]; then\n";
        std::cout << "                    echo \"${COMP_WORDS[$((i+1))]}\"\n";
        std::cout << "                    return\n";
        std::cout << "                fi\n";
        std::cout << "                ;;\n";
        std::cout << "        esac\n";
        std::cout << "    done\n";
        std::cout << "    echo \".\"\n";
        std::cout << "}\n";
        std::cout << "\n";

        // Helper function to get tileset completions
        std::cout << "# Helper function to get tileset completions\n";
        std::cout << "_porytiles2_complete_tilesets() {\n";
        std::cout << "    local filter=\"$1\"\n";
        std::cout << "    local project_root\n";
        std::cout << "    project_root=$(_porytiles2_get_project_root)\n";
        std::cout << "    porytiles2 list-tilesets -C \"$project_root\" --filter \"$filter\" --prefix \"$cur\" "
                     "2>/dev/null\n";
        std::cout << "}\n";
        std::cout << "\n";

        std::cout << "_porytiles2_completions() {\n";
        std::cout << "    local cur prev opts subcommand\n";
        std::cout << "    COMPREPLY=()\n";
        std::cout << "    cur=\"${COMP_WORDS[COMP_CWORD]}\"\n";
        std::cout << "    prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n";
        std::cout << "\n";

        // Find the subcommand
        std::cout << "    # Find the subcommand\n";
        std::cout << "    subcommand=\"\"\n";
        std::cout << "    for ((i=1; i < COMP_CWORD; i++)); do\n";
        std::cout << "        case \"${COMP_WORDS[i]}\" in\n";
        std::cout << "            compile-tileset|create-tileset|decompile-tileset|dump-tileset-config|"
                     "import-tileset|list-tilesets|completion)\n";
        std::cout << "                subcommand=\"${COMP_WORDS[i]}\"\n";
        std::cout << "                break\n";
        std::cout << "                ;;\n";
        std::cout << "        esac\n";
        std::cout << "    done\n";
        std::cout << "\n";

        std::cout << "    # Main commands\n";
        std::cout << "    local commands=\"compile-tileset create-tileset decompile-tileset dump-tileset-config "
                     "import-tileset list-tilesets completion\"\n";
        std::cout << "\n";
        std::cout << "    # Config options\n";
        std::cout << "    local config_opts=\"";

        auto meta = porytiles2::get_cli_option_metadata();
        for (const auto &opt : meta) {
            std::cout << "--" << opt.long_name << " ";
            if (opt.has_negation) {
                std::cout << "--no-" << opt.long_name << " ";
            }
        }

        std::cout << "\"\n";
        std::cout << "\n";

        // Handle completion based on context
        std::cout << "    # If no subcommand yet, complete commands\n";
        std::cout << "    if [[ -z \"$subcommand\" ]]; then\n";
        std::cout << "        if [[ ${cur} == -* ]]; then\n";
        std::cout << "            COMPREPLY=( $(compgen -W \"-C --project-root\" -- ${cur}) )\n";
        std::cout << "        else\n";
        std::cout << "            COMPREPLY=( $(compgen -W \"${commands}\" -- ${cur}) )\n";
        std::cout << "        fi\n";
        std::cout << "        return 0\n";
        std::cout << "    fi\n";
        std::cout << "\n";

        std::cout << "    case \"${prev}\" in\n";

        // Add case statements for enum options
        for (const auto &opt : meta) {
            if (!opt.choices.empty()) {
                std::cout << "        --" << opt.long_name << ")\n";
                std::cout << "            COMPREPLY=( $(compgen -W \"";
                for (std::size_t i = 0; i < opt.choices.size(); ++i) {
                    if (i > 0) {
                        std::cout << " ";
                    }
                    std::cout << opt.choices[i];
                }
                std::cout << "\" -- ${cur}) )\n";
                std::cout << "            return 0\n";
                std::cout << "            ;;\n";
            }
        }

        std::cout << "        -C|--project-root)\n";
        std::cout << "            # Complete directories\n";
        std::cout << "            COMPREPLY=( $(compgen -d -- ${cur}) )\n";
        std::cout << "            return 0\n";
        std::cout << "            ;;\n";
        std::cout << "        *)\n";
        std::cout << "            ;;\n";
        std::cout << "    esac\n";
        std::cout << "\n";

        // Options completion
        std::cout << "    if [[ ${cur} == -* ]]; then\n";
        std::cout << "        COMPREPLY=( $(compgen -W \"${config_opts}\" -- ${cur}) )\n";
        std::cout << "        return 0\n";
        std::cout << "    fi\n";
        std::cout << "\n";

        // Tileset name completion based on subcommand
        std::cout << "    # Complete tileset names based on subcommand\n";
        std::cout << "    case \"$subcommand\" in\n";
        std::cout << "        compile-tileset|decompile-tileset)\n";
        std::cout << "            # Only managed tilesets\n";
        std::cout << "            COMPREPLY=( $(_porytiles2_complete_tilesets managed) )\n";
        std::cout << "            ;;\n";
        std::cout << "        dump-tileset-config)\n";
        std::cout << "            # All tilesets (config can be dumped for any)\n";
        std::cout << "            COMPREPLY=( $(_porytiles2_complete_tilesets all) )\n";
        std::cout << "            ;;\n";
        std::cout << "        import-tileset)\n";
        std::cout << "            # Only unmanaged tilesets\n";
        std::cout << "            COMPREPLY=( $(_porytiles2_complete_tilesets unmanaged) )\n";
        std::cout << "            ;;\n";
        std::cout << "        create-tileset)\n";
        std::cout << "            # No completion - user provides new name\n";
        std::cout << "            ;;\n";
        std::cout << "        list-tilesets)\n";
        std::cout << "            # Complete --filter values\n";
        std::cout << "            if [[ \"${prev}\" == \"--filter\" ]]; then\n";
        std::cout << "                COMPREPLY=( $(compgen -W \"all managed unmanaged\" -- ${cur}) )\n";
        std::cout << "            fi\n";
        std::cout << "            ;;\n";
        std::cout << "        completion)\n";
        std::cout << "            COMPREPLY=( $(compgen -W \"bash zsh fish\" -- ${cur}) )\n";
        std::cout << "            ;;\n";
        std::cout << "    esac\n";
        std::cout << "\n";
        std::cout << "    return 0\n";
        std::cout << "}\n";
        std::cout << "\n";
        std::cout << "complete -F _porytiles2_completions porytiles2\n";
    }

    void output_zsh_completion()
    {
        std::cout << "#compdef porytiles2\n";
        std::cout << "# Zsh completion for porytiles2\n";
        std::cout << "# Add to ~/.zshrc or place in a directory in your $fpath:\n";
        std::cout << "#   eval \"$(porytiles2 completion zsh)\"\n";
        std::cout << "\n";

        // Helper function to extract project root
        std::cout << "# Helper function to extract --project-root or -C from command line\n";
        std::cout << "_porytiles2_get_project_root() {\n";
        std::cout << "    local i\n";
        std::cout << "    for ((i=1; i <= ${#words[@]}; i++)); do\n";
        std::cout << "        case \"${words[i]}\" in\n";
        std::cout << "            -C|--project-root)\n";
        std::cout << "                if [[ $((i+1)) -le ${#words[@]} ]]; then\n";
        std::cout << "                    echo \"${words[$((i+1))]}\"\n";
        std::cout << "                    return\n";
        std::cout << "                fi\n";
        std::cout << "                ;;\n";
        std::cout << "        esac\n";
        std::cout << "    done\n";
        std::cout << "    echo \".\"\n";
        std::cout << "}\n";
        std::cout << "\n";

        // Helper function for tileset completion
        std::cout << "# Helper function to complete tileset names\n";
        std::cout << "_porytiles2_complete_tilesets() {\n";
        std::cout << "    local filter=\"$1\"\n";
        std::cout << "    local project_root\n";
        std::cout << "    project_root=$(_porytiles2_get_project_root)\n";
        std::cout << "    local -a tilesets\n";
        std::cout << "    tilesets=(${(f)\"$(porytiles2 list-tilesets -C \"$project_root\" --filter \"$filter\" "
                     "2>/dev/null)\"})\n";
        std::cout << "    _describe 'tileset' tilesets\n";
        std::cout << "}\n";
        std::cout << "\n";

        std::cout << "_porytiles2() {\n";
        std::cout << "    local state line\n";
        std::cout << "    local -a commands\n";
        std::cout << "    commands=(\n";
        std::cout << "        'compile-tileset:Compile a tileset'\n";
        std::cout << "        'create-tileset:Create a new tileset'\n";
        std::cout << "        'decompile-tileset:Decompile a tileset'\n";
        std::cout << "        'dump-tileset-config:Dump the full configuration provenance chain for a tileset'\n";
        std::cout << "        'import-tileset:Import a pre-existing tileset'\n";
        std::cout << "        'list-tilesets:List tileset names in the project'\n";
        std::cout << "        'completion:Generate shell completion scripts'\n";
        std::cout << "    )\n";
        std::cout << "\n";
        std::cout << "    local -a config_opts\n";
        std::cout << "    config_opts=(\n";
        std::cout << "        '-C[Set project root directory]:directory:_files -/'\n";
        std::cout << "        '--project-root[Set project root directory]:directory:_files -/'\n";

        auto meta = porytiles2::get_cli_option_metadata();
        for (const auto &opt : meta) {
            std::cout << "        '--" << opt.long_name << "[" << opt.description << "]";
            if (!opt.choices.empty()) {
                std::cout << ":choice:(";
                for (std::size_t i = 0; i < opt.choices.size(); ++i) {
                    if (i > 0) {
                        std::cout << " ";
                    }
                    std::cout << opt.choices[i];
                }
                std::cout << ")";
            }
            std::cout << "'\n";
            if (opt.has_negation) {
                std::cout << "        '--no-" << opt.long_name << "[Disable " << opt.description << "]'\n";
            }
        }

        std::cout << "    )\n";
        std::cout << "\n";
        std::cout << "    _arguments -C \\\n";
        std::cout << "        '-C[Set project root directory]:directory:_files -/' \\\n";
        std::cout << "        '--project-root[Set project root directory]:directory:_files -/' \\\n";
        std::cout << "        '1:command:->command' \\\n";
        std::cout << "        '*::arg:->args'\n";
        std::cout << "\n";
        std::cout << "    case \"$state\" in\n";
        std::cout << "        command)\n";
        std::cout << "            _describe 'command' commands\n";
        std::cout << "            ;;\n";
        std::cout << "        args)\n";
        std::cout << "            case \"$line[1]\" in\n";
        std::cout << "                compile-tileset|decompile-tileset)\n";
        std::cout << "                    _arguments \\\n";
        std::cout << "                        $config_opts \\\n";
        std::cout << "                        '1:tileset:->tileset_managed'\n";
        std::cout
            << "                    [[ \"$state\" == tileset_managed ]] && _porytiles2_complete_tilesets managed\n";
        std::cout << "                    ;;\n";
        std::cout << "                dump-tileset-config)\n";
        std::cout << "                    _arguments \\\n";
        std::cout << "                        $config_opts \\\n";
        std::cout << "                        '1:tileset:->tileset_all'\n";
        std::cout << "                    [[ \"$state\" == tileset_all ]] && _porytiles2_complete_tilesets all\n";
        std::cout << "                    ;;\n";
        std::cout << "                import-tileset)\n";
        std::cout << "                    _arguments \\\n";
        std::cout << "                        $config_opts \\\n";
        std::cout << "                        '1:tileset:->tileset_unmanaged'\n";
        std::cout
            << "                    [[ \"$state\" == tileset_unmanaged ]] && _porytiles2_complete_tilesets unmanaged\n";
        std::cout << "                    ;;\n";
        std::cout << "                create-tileset)\n";
        std::cout << "                    _arguments \\\n";
        std::cout << "                        $config_opts \\\n";
        std::cout << "                        '1:tileset name:'\n";
        std::cout << "                    ;;\n";
        std::cout << "                list-tilesets)\n";
        std::cout << "                    _arguments \\\n";
        std::cout << "                        '-C[Set project root directory]:directory:_files -/' \\\n";
        std::cout << "                        '--project-root[Set project root directory]:directory:_files -/' \\\n";
        std::cout << "                        '--filter[Filter mode]:filter:(all managed unmanaged)' \\\n";
        std::cout << "                        '--prefix[Only show tilesets starting with this prefix]:prefix:'\n";
        std::cout << "                    ;;\n";
        std::cout << "                completion)\n";
        std::cout << "                    _arguments '1:shell:(bash zsh fish)'\n";
        std::cout << "                    ;;\n";
        std::cout << "                *)\n";
        std::cout << "                    _values 'config options' $config_opts\n";
        std::cout << "                    ;;\n";
        std::cout << "            esac\n";
        std::cout << "            ;;\n";
        std::cout << "    esac\n";
        std::cout << "}\n";
        std::cout << "\n";
        std::cout << "compdef _porytiles2 porytiles2\n";
    }

    void output_fish_completion()
    {
        std::cout << "# Fish completion for porytiles2\n";
        std::cout << "# Save to completions directory (eval does not work reliably in fish):\n";
        std::cout << "#   porytiles2 completion fish > ~/.config/fish/completions/porytiles2.fish\n";
        std::cout << "# Then restart fish or run: source ~/.config/fish/completions/porytiles2.fish\n";
        std::cout << "\n";
        std::cout << "# Erase any existing completions for porytiles2\n";
        std::cout << "complete -e -c porytiles2\n";
        std::cout << "\n";
        std::cout << "# Disable file completions globally for porytiles2\n";
        std::cout << "complete -c porytiles2 -f\n";
        std::cout << "\n";

        // Helper function to get project root
        std::cout << "# Helper function to extract --project-root or -C from command line\n";
        std::cout << "function __porytiles2_get_project_root\n";
        std::cout << "    set -l tokens (commandline -opc)\n";
        std::cout << "    for i in (seq 1 (count $tokens))\n";
        std::cout << "        switch $tokens[$i]\n";
        std::cout << "            case -C --project-root\n";
        std::cout << "                set -l next (math $i + 1)\n";
        std::cout << "                if test $next -le (count $tokens)\n";
        std::cout << "                    echo $tokens[$next]\n";
        std::cout << "                    return\n";
        std::cout << "                end\n";
        std::cout << "        end\n";
        std::cout << "    end\n";
        std::cout << "    echo \".\"\n";
        std::cout << "end\n";
        std::cout << "\n";

        // Helper function for tileset completion
        std::cout << "# Helper function to complete tileset names\n";
        std::cout << "function __porytiles2_complete_tilesets\n";
        std::cout << "    set -l filter $argv[1]\n";
        std::cout << "    set -l project_root (__porytiles2_get_project_root)\n";
        std::cout << "    porytiles2 list-tilesets -C \"$project_root\" --filter \"$filter\" 2>/dev/null\n";
        std::cout << "end\n";
        std::cout << "\n";

        // Helper functions for context detection
        std::cout << "# Helper functions for detecting current subcommand\n";
        std::cout << "function __porytiles2_needs_subcommand\n";
        std::cout << "    set -l cmd (commandline -opc)\n";
        std::cout << "    for word in $cmd[2..-1]\n";
        std::cout << "        switch $word\n";
        std::cout << "            case compile-tileset create-tileset decompile-tileset dump-tileset-config "
                     "import-tileset list-tilesets completion\n";
        std::cout << "                return 1\n";
        std::cout << "        end\n";
        std::cout << "    end\n";
        std::cout << "    return 0\n";
        std::cout << "end\n";
        std::cout << "\n";

        std::cout << "function __porytiles2_using_subcommand\n";
        std::cout << "    set -l cmd (commandline -opc)\n";
        std::cout << "    for word in $cmd[2..-1]\n";
        std::cout << "        if test \"$word\" = \"$argv[1]\"\n";
        std::cout << "            return 0\n";
        std::cout << "        end\n";
        std::cout << "    end\n";
        std::cout << "    return 1\n";
        std::cout << "end\n";
        std::cout << "\n";

        // Commands
        std::cout << "# Commands\n";
        std::cout << "complete -c porytiles2 -f -n __porytiles2_needs_subcommand -a compile-tileset -d 'Compile a "
                     "tileset'\n";
        std::cout << "complete -c porytiles2 -f -n __porytiles2_needs_subcommand -a create-tileset -d 'Create a new "
                     "tileset'\n";
        std::cout << "complete -c porytiles2 -f -n __porytiles2_needs_subcommand -a decompile-tileset -d 'Decompile a "
                     "tileset'\n";
        std::cout << "complete -c porytiles2 -f -n __porytiles2_needs_subcommand -a dump-tileset-config -d 'Dump the "
                     "full configuration provenance chain for a tileset'\n";
        std::cout << "complete -c porytiles2 -f -n __porytiles2_needs_subcommand -a import-tileset -d 'Import a "
                     "pre-existing tileset'\n";
        std::cout << "complete -c porytiles2 -f -n __porytiles2_needs_subcommand -a list-tilesets -d 'List tileset "
                     "names in the project'\n";
        std::cout << "complete -c porytiles2 -f -n __porytiles2_needs_subcommand -a completion -d 'Generate shell "
                     "completion scripts'\n";
        std::cout << "\n";

        // Global options
        std::cout << "# Global options\n";
        std::cout << "complete -c porytiles2 -f -s C -l project-root -d 'Set project root directory' -xa '(__fish_"
                     "complete_directories)'\n";
        std::cout << "\n";

        // Tileset name completions per subcommand
        std::cout << "# Tileset name completions\n";
        std::cout << "complete -c porytiles2 -f -n '__porytiles2_using_subcommand compile-tileset' -a "
                     "'(__porytiles2_complete_tilesets managed)'\n";
        std::cout << "complete -c porytiles2 -f -n '__porytiles2_using_subcommand decompile-tileset' -a "
                     "'(__porytiles2_complete_tilesets managed)'\n";
        std::cout << "complete -c porytiles2 -f -n '__porytiles2_using_subcommand dump-tileset-config' -a "
                     "'(__porytiles2_complete_tilesets all)'\n";
        std::cout << "complete -c porytiles2 -f -n '__porytiles2_using_subcommand import-tileset' -a "
                     "'(__porytiles2_complete_tilesets unmanaged)'\n";
        std::cout << "\n";

        // Options for list-tilesets subcommand
        std::cout << "# Options for list-tilesets subcommand\n";
        std::cout << "complete -c porytiles2 -f -n '__porytiles2_using_subcommand list-tilesets' -l filter -d "
                     "'Filter mode' -xa 'all managed unmanaged'\n";
        std::cout << "complete -c porytiles2 -f -n '__porytiles2_using_subcommand list-tilesets' -l prefix -d "
                     "'Only show tilesets starting with this prefix'\n";
        std::cout << "\n";

        // Completion for the 'completion' subcommand
        std::cout << "# Shell type completion for 'completion' subcommand\n";
        std::cout << "complete -c porytiles2 -f -n '__porytiles2_using_subcommand completion' -a 'bash zsh fish'\n";
        std::cout << "\n";

        // Config options
        std::cout << "# Config options\n";

        auto meta = porytiles2::get_cli_option_metadata();
        for (const auto &opt : meta) {
            std::cout << "complete -c porytiles2 -f -l " << opt.long_name << " -d '" << opt.description << "'";
            if (!opt.choices.empty()) {
                std::cout << " -xa '";
                for (std::size_t i = 0; i < opt.choices.size(); ++i) {
                    if (i > 0) {
                        std::cout << " ";
                    }
                    std::cout << opt.choices[i];
                }
                std::cout << "'";
            }
            std::cout << "\n";
            if (opt.has_negation) {
                std::cout << "complete -c porytiles2 -f -l no-" << opt.long_name << " -d 'Disable " << opt.description
                          << "'\n";
            }
        }
    }
};
