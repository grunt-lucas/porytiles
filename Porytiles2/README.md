# Porytiles 2
**Major Version 2** (MV2) is the next-generation Porytiles offering,
featuring significant enhancements and a very different UX.
Users can access MV2 functionality via the new driver, `porytiles2`.
MV2's code lives in this `Porytiles2` directory,
and the driver code lives in the `tools/driver` subtree.
MV2 is tested via GoogleTest, which lives in the `tests` subtree.

Unlike MV1, MV2 uses a library-based architecture,
which should make it much easier to integrate with other tooling.
Our long-term goal is to integrate the core Porytiles functionality
directly into Porymap.
