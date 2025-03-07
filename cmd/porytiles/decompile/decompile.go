/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package decompile

import (
	"fmt"

	"github.com/spf13/cobra"
)

var DecompileCmd = &cobra.Command{
	Use:   "decompile",
	Short: "Decompile a tileset, spritesheet, or map",
	Long: `Decompile a tileset, spritesheet, or map. Decompilation generally transforms
Porymap-format assets into Porytiles-format assets.`,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("decompile called")
	},
}

func init() {
	// Here you will define your flags and configuration settings.

	// Cobra supports Persistent Flags which will work for this command
	// and all subcommands, e.g.:
	// compileCmd.PersistentFlags().String("foo", "", "A help for foo")

	// Cobra supports local flags which will only run when this command
	// is called directly, e.g.:
	// compileCmd.Flags().BoolP("toggle", "t", false, "Help message for toggle")
}
