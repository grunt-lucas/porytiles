/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package compile

import (
	"fmt"

	"github.com/spf13/cobra"
)

var spritesheetCmd = &cobra.Command{
	Use:   "spritesheet",
	Short: "Compile RGBA spritesheets into a 'tiles.png' and pal files",
	Long: `A longer description that spans multiple lines and likely contains examples
and usage of using your command. For example:

Cobra is a CLI library for Go that empowers applications.
This application is a tool to generate the needed files
to quickly create a Cobra application.`,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("spritesheet called")
	},
}

func init() {
	CompileCmd.AddCommand(spritesheetCmd)

	// Here you will define your flags and configuration settings.

	// Cobra supports Persistent Flags which will work for this command
	// and all subcommands, e.g.:
	// spritesheetCmd.PersistentFlags().String("foo", "", "A help for foo")

	// Cobra supports local flags which will only run when this command
	// is called directly, e.g.:
	// spritesheetCmd.Flags().BoolP("toggle", "t", false, "Help message for toggle")
}
