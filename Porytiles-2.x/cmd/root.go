/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/
package cmd

import (
	"os"

	"github.com/grunt-lucas/porytiles/cmd/compile"
	"github.com/spf13/cobra"
)

// rootCmd represents the base command when called without any subcommands
var rootCmd = &cobra.Command{
	Use:   "porytiles",
	Short: "Overworld tileset compiler for Pokémon Generation III decompilation projects",
	Long: `Overworld tileset compiler for use with the pokeruby, pokefirered, and
pokeemerald Pokémon Generation III decompilation projects from pret. Also
compatible with pokeemerald-expansion from rh-hideout. Builds Porymap-ready
tilesets from RGBA (or indexed) tile assets.

Project home page: https://github.com/grunt-lucas/porytiles`,
}

// Execute adds all child commands to the root command and sets flags appropriately.
// This is called by main.main(). It only needs to happen once to the rootCmd.
func Execute() {
	err := rootCmd.Execute()
	if err != nil {
		os.Exit(1)
	}
}

func init() {
	rootCmd.AddCommand(compile.CompileCmd)
	rootCmd.Version = "2.x"
	// Here you will define your flags and configuration settings.
	// Cobra supports persistent flags, which, if defined here,
	// will be global for your application.

	// rootCmd.PersistentFlags().StringVar(&cfgFile, "config", "", "config file (default is $HOME/.porytiles.yaml)")

	// Cobra also supports local flags, which will only run
	// when this action is called directly.
	rootCmd.Flags().BoolP("toggle", "t", false, "Help message for toggle")
}
