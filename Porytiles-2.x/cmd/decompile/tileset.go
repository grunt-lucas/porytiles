/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/
package decompile

import (
	"fmt"

	"github.com/spf13/cobra"
)

var tilesetCmd = &cobra.Command{
	Use:   "tileset",
	Short: "Decompile a Porymap-format tileset to a Porytiles-format tileset",
	Long:  `Decompile a Porymap-format tileset to a Porytiles-format tileset.`,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("tileset called")
	},
}

func init() {
	DecompileCmd.AddCommand(tilesetCmd)

	// Here you will define your flags and configuration settings.

	// Cobra supports Persistent Flags which will work for this command
	// and all subcommands, e.g.:
	// tilesetCmd.PersistentFlags().String("foo", "", "A help for foo")

	// Cobra supports local flags which will only run when this command
	// is called directly, e.g.:
	// tilesetCmd.Flags().BoolP("toggle", "t", false, "Help message for toggle")
}
