/*
Copyright © 2024 grunt-lucas grunt.lucas@yahoo.com
*/
package compile

import (
	"fmt"

	"github.com/spf13/cobra"
)

var CompileCmd = &cobra.Command{
	Use:   "compile",
	Short: "Compile a tileset, spritesheet, or map",
	Long: `Compile a tileset, spritesheet, or map. Compilation generally transforms
Porytiles-format assets into Porymap-format assets.

Tileset compilation is the standard Porytiles workflow that transforms a
complete Porytiles-format tileset into a complete Porymap-format tileset. The
input PNGs in tileset mode must conform to a few constraints:

  - exactly 128 pixels (i.e. 8 metatiles) in width

  - a multiple of 16 (i.e. an integral number of metatiles) in height

Spritesheet compilation is a stripped down compilation process that doesn't
produce any metatile binary information. In spritesheet mode, the compiler
accepts an arbitrary number of PNGs with arbitrary dimensions. It will walk the
PNGs tile-by-tile and generate a deduped / deflipped 'tiles.png' as well as
however many palette files are necessary to assign all the tiles. It will also
generate a 'key.txt' file so you can easily see which tiles in 'tiles.png'
correspond to the original tiles on your spritesheet, and to which palette these
tiles should be assigned. Spritesheet mode is useful if you have a bunch of
pixel art you want to tile-ize, but you don't mind painting metatiles yourself
in Porymap.

Map compilation is
TODO : fill in documentation

A Porytiles-format tileset is a directory containing, at minimum: a bottom,
middle, and top RGBA PNG file representing the bottom, middle, and top
background layers respectively. It may also optionally include:

  - an 'attributes.csv' file, if you want Porytiles to manage the various
    metatile attributes for your target base game

  - an 'anim' folder containing RGBA PNG animation frames, if you want Porytiles
    to manage your tilesets animations

  - a 'palette-primers' folder containing primer JASC .pal files, if you want
    to prime Porytiles's palette allocation algorithm

A Porymap-format tileset is a directory containing assets that can be imported
directly into Porymap as a ready-to-use tileset. These assets include:

  - a 'tiles.png' file in 4-bit PNG indexed mode

  - a 'metatiles.bin' file containing the metatile binary data

  - a 'metatile_attributes.bin' file containing metatile attribute binary data

  - a 'palettes' folder containing 16 JASC .pal files, numbered 0 to 15

  - an 'anim' folder containing greyscale indexed animation frame PNGs, where
    each animation is stored in a named subdirectory

See these Porytiles wiki pages for more information about the above topics:
  https://github.com/grunt-lucas/porytiles/wiki/Compiling-A-Primary-Tileset
  https://github.com/grunt-lucas/porytiles/wiki/Compiling-A-Secondary-Tileset
  https://github.com/grunt-lucas/porytiles/wiki/Adding-Animations
  https://github.com/grunt-lucas/porytiles/wiki/Metatile-Attributes
  https://github.com/grunt-lucas/porytiles/wiki/Palette-Primers
`,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("compile called")
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
