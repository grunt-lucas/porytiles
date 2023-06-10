# Porytiles

Create a tileset and palettes from a master image.

### Who/what is this for?

`porytiles` is for users who just want to get some art into the game quickly. The palette allocation algorithm is greedy
and naive, so some of the heuristics it uses to construct the palettes are not always optimal. Power users will almost
always be better off manually constructing their big primary tilesets. However, `porytiles` may have a place to help
speed along tileset creation or to automatically generate the tilesets for less intensive areas that don't need to
completely optimize palette space. If you have many maps that you know only need 3-4 of the 5-6 secondary palettes
available, `porytiles` will save you a ton of time and headache.
