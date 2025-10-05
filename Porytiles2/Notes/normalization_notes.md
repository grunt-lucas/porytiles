- [ISOMORPHISM](#isomorphism)
  - [Isomorphism Under Flip Transformations](#isomorphism-under-flip-transformations)
  - [Isomorphism Under Color Transformations](#isomorphism-under-color-transformations)
  - [Neither Isomorphism Is A Special Case Of The Other](#neither-isomorphism-is-a-special-case-of-the-other)
- [NORMALIZATION](#normalization)
  - [RGBA Tiles Set 1](#rgba-tiles-set-1)
    - [Old Normalized Tiles](#old-normalized-tiles)
  - [RGBA Tiles Set 2](#rgba-tiles-set-2)
    - [Old Normalized Tiles](#old-normalized-tiles-1)
    - [Old GBA Tiles](#old-gba-tiles)
    - [New Normalized Tiles](#new-normalized-tiles)
    - [New GBA Tiles](#new-gba-tiles)
- [We need something better](#we-need-something-better)
  - [Old Technique](#old-technique)
  - [New Technique](#new-technique)

# ISOMORPHISM
Ultimately, tiles can be isomorphic in two ways.

## Isomorphism Under Flip Transformations
Suppose we have two tiles, Tile A and Tile B.
If there exists a function F which consists of a finite sequence of flip transformations,
such that F(A) = B,
then Tile A and Tile B are isomorphic under flip transformation F.

This is the original "problem" normalization attempts to solve,
since these tiles should ALWAYS be able to share a GBA tile.

## Isomorphism Under Color Transformations
There is a second way two tiles can be isomorphic.

Suppose we have two tiles, Tile A and Tile B.
Let F be a function from colors to colors where:

1. The domain of F at least the set of all unique RGBA colors in A
2. The range of F is at least the set of unique RGBA colors in B
3. Applying F to each pixel in Tile A yields Tile B

Then Tile A and Tile B are isomorphic under color transformation F.

This is what we colloquially call "sibling" tiles.
A classic example: the Pokemart and Pokecenter roof tiles.
Same shape, different palettes.
The vanilla game uses the same GBA tile with cleverly aligned palettes to make this work.

## Takeaways
Any given pair of rgba tiles will have one of the following states:
1. not isomorphic
2. isomorphic under flip only
3. isomorphic under color only
4. isomorphic under both flip and color

This final state, state 4, is what gave rise to https://github.com/grunt-lucas/porytiles/issues/118
in the original Porytiles.
The way the original normalization system interacted with the GBA maketile function
caused it to fail to generate the correct GBA tile for this case.

# NORMALIZATION
Normalization is the Porytiles process by which it tries to construct a tile representation
such that two tiles which are isomorphic have "equivalent" normal forms.

Thus far, neither of the normal form computation processes I have developed are fully sufficient.

## RGBA Tiles Set 1
This first set is designed to show the legacy normalization working as intended for iso-under-flips.
It also shows how the new normalization technique works similarly, just with slightly different intermediate steps.
These two tiles are iso-under-flips.

```
R B G B    B G B R    C Y M Y    Y M Y C
B B G B    B G B B    Y Y M Y    Y M Y Y
B B B B    B B B B    Y Y Y Y    Y Y Y Y
B B B B    B B B B    Y Y Y Y    Y Y Y Y
```

### Old Normalized Tiles
```
1 1 1 1    1 1 1 1
1 1 1 1    1 1 1 1
1 1 2 1    1 1 2 1
3 1 2 1    3 1 2 1
ft         tt
1 2 3      1 2 3
B G R      B G R
```

In this case, the legacy normalization algo is working as expected.
Luckily, they have the same NormPix and NormPal, just different flip bits.


## RGBA Tiles Set 2
This set is designed to showcase how https://github.com/grunt-lucas/porytiles/issues/118 arises in certain cases.
It also shows how the new normalization method fixes this particular case.

Tile 1 and 2 are iso-under-flip. Tile 3 is iso-under-color with both 1 and 2, just using slightly different functions.

```
B < G < R < C < M < Y
0,0,255 < 0,255,0 < 255,0,0 < 0,255,255 < 255,0,255 < 255,255,0
```

```
R B B    G B B    C M M
B B B    B B B    M M M
B B G    B B R    M M Y
```

### Old Normalized Tiles
```
1 2 2    1 2 2    1 2 2
2 2 2    2 2 2    2 2 2
2 2 3    2 2 3    2 2 3
ff       ff       ff
1 2 3    1 2 3    1 2 3
-----    -----    -----
R B G    G B R    C M Y
```

Under the old normalization system,
all three of these tiles have the same NormPix.
In fact, the old normalization system will always
generate identical NormPix for tiles that are isomorphic under either aspect.
However as you can see below, since the NormPals can be different different,
the GBA tiles for these tiles won't always be identical.
Even though it seems obvious that Tile 1 and 2 should be able to share GBA tiles.

### Old GBA Tiles
```
1 2 2    3 2 2    1 2 2
2 2 2    2 2 2    2 2 2
2 2 3    2 2 1    2 2 3

Pal 1    Pal 2
1 2 3    1 2 3
R B G    C M Y
```

Notice how the GBA tiles for Tile 1 and 2 differ?
This is because the NormPals are different.
And even though the NormPix are identical,
when forced to index into a fixed GBA pal,
it ends up with different GBA tiles.

However, we have aligned Pal 1 and Pal 2
such that Tile 1 and 3 can share the same GBA tile.

### New Normalized Tiles
```
2 1 1    2 1 1    1 2 2
1 1 1    1 1 1    2 2 2
1 1 3    1 1 3    2 2 3
tt       ff       ff
1 2 3    1 2 3    1 2 3
-----    -----    -----
B G R    B G R    C M Y
```

As you can see, with the new normalization system,
we lose the property that isomorphic-under-color tiles always have the same NormPix.
However, it definitely solves the bug
wherein some isomorphic-under-flip tiles can end up with different GBA tiles.

### New GBA Tiles
```
2 1 1    2 1 1    2 1 1
1 1 1    1 1 1    1 1 1
1 1 3    1 1 3    1 1 3

Pal 1    Pal 2
1 2 3    1 2 3
B G R    M C Y
```

Now GBA tiles for Tile 1 and 2 are identical as expected.
Since the NormPals are identical,
and the NormPix are identical,
the GBA tiles are identical.

We also set up Pal 2 so that Tile 3 could share the same GBA tile as Tiles 1 and 2.
The problem is that it's not obvious how to set up the pals this way in every case.

# We need something better
Is there some way to develop a normalization scheme such that
it's easy to check if two tiles are isomorphic under either or both aspects?

Or should NormalizedTile contain multiple "normalization" methods which store pixels that
are indentical under different isomorphisms?

E.g. we have an iso-under-flips pix and pal, which use the new method
which guarantees that iso-under-flips have same pix and pal.
Then, we can have a NormalizedTile::isomorphic_under_flip(NormalizedTile&)
which checks if the other tile's pix and pal match this one.

## Old Technique
```
          | iso-under-flip                | iso-under-color
-------------------------------------------------------------
pix       | always identical              | always identical 
--------- | -------------------------------------------------
pal       | usually identical, not always | always different
--------- | -------------------------------------------------
flip bits | often different               | sometimes different
```

As you can see from this table, the old technique is not really useful for differentiating between the two iso cases.
If two tiles have the same NormPix, you can't tell under which transformation they are isomorphic.
You have to look at the NormPal. Most (but not all) iso-under-flip tiles will have identical NormPals.

## New Technique
```
          | iso-under-flip      | iso-under-color
-------------------------------------------------
pix       | always identical    | ???
--------- | -------------------------------------
pal       | always identical    | ???
--------- | -------------------------------------
flip bits | often different     | ???
```

As you can see from this table, the old technique is not really useful for differentiating between the two iso cases.
If two tiles have the same NormPix, you can't tell under which transformation they are isomorphic.
You have to look at the NormPal. Most (but not all) iso-under-flip tiles will have identical NormPals.

# Here's An Idea

## Isomorphism Under Color Transformation
Create a new tile type: `IsoColorTile`.

`IsoColorTile`
- `Tile<IndexPixel> tile`
- `std::vector<Rgba32> pal`
- `bool flip bits`

`IsoColorTileNormalizer`: construct `IsoColorTile`s using the "old" normalization algorithm.

`bool is_isomorphic(IsoColorTile&)`
- tile pixels are identical
- pals are not identical
  - pal are not a simple reordering: push pals into set, if sets are the same, pals are a shuffle

We can compute the function F by aligning the pals of the two tiles.
Suppose pal 1 is [R, G, B] and pal 2 is [C, M, Y].
We can conclude that F(R) = C, F(G) = M, F(B) = Y.

The case where the pals are just a reordering of each other
is the classic Issue 118 case where two tiles are isomorphic under both color and flip.
In these cases, we'd rather treat these tiles as iso-under-flip only.

## Isomorphism Under Flip Transformation
Create a new tile type: `IsoFlipTile`.

`IsoFlipTile`
- `Tile<IndexPixel> tile`
- `std::set<Rgba32> pal`
- `bool flip bits`

`IsoFlipTileNormalizer`: construct `IsoFlipTile`s using the "new" normalization algorithm,
already defined in `RgbaTileNormalizer`.

`bool is_isomorphic(IsoFlipTile&)`
- tile pixels are identical
- pals are identical
