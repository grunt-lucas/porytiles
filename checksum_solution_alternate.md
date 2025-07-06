# Checksum Handling Design Proposal

This document proposes a design for handling artifact checksums in Porytiles2, addressing the `TODO` comment in `Porytiles2/include/porytiles2/domain/repos/TilesetRepo.hpp`.

## 1. The Problem

The `TilesetRepo.hpp` `TODO` raises a key design question: How should we manage artifact checksums? It correctly observes that checksums seem to be a persistence-layer concern rather than a core domain-layer concern. The goal is to find a design that supports the use cases outlined in `ARCHITECTURE.md` without polluting the `Tileset` aggregate with infrastructure-related data.

## 2. Proposed Design

The proposed solution is rooted in the principle that **checksums are metadata about the persisted state and belong to the persistence layer**. The `Tileset` aggregate should remain pure and ignorant of how its compiled representation is stored or verified on the filesystem.

The responsibility for managing checksums will be given entirely to the `TilesetRepo`.

### Key Principles:

1.  **Domain Purity**: The `Tileset` aggregate will not contain any checksum data. Its responsibility is to enforce domain invariants and manage the state of the tileset's constituent parts (layers, palettes, animations, etc.).
2.  **Repository Responsibility**: The `TilesetRepo` implementation is responsible for all persistence operations. This includes reading/writing the Porymap binary artifacts (`metatiles.bin`, `tiles.png`, etc.) and managing the `artifact_checksums.json` file that stores their checksums.
3.  **Application Layer Orchestration**: The application services (which implement the use cases) will orchestrate the process, using the repository's capabilities to perform the necessary checks before executing domain logic.

### 3. `TilesetRepo` Interface Changes

To implement this, the `TilesetRepo` interface will be updated as follows:

```cpp
// Porytiles2/include/porytiles2/domain/repos/TilesetRepo.hpp

class TilesetRepo {
public:
  virtual ~TilesetRepo() = default;

  /**
   * @brief Persists a new or existing Tileset.
   *
   * @details
   * The implementation of this method is responsible for writing all the
   * Porymap assets (metatiles.bin, tiles.png, etc.) to the filesystem.
   * After writing the assets, it must compute their checksums and atomically
   * update the `artifact_checksums.json` file as part of the same logical
   * persistence operation.
   *
   * @param tileset The Tileset to save.
   * @return An empty Result on success, otherwise an error description.
   */
  [[nodiscard]] virtual Result<void> Save(const Tileset &tileset) = 0;

  /**
   * @brief Loads an existing Tileset from storage.
   *
   * @param name The name of the Tileset to load.
   * @return A Tileset Result on success, otherwise an error description.
   */
  [[nodiscard]] virtual Result<std::unique_ptr<Tileset>> Load(const std::string &name) = 0;

  /**
   * @brief Checks if the given Tileset exists in the backing store.
   *
   * @param name The name of the Tileset to check.
   * @return True if the named tileset exists, false otherwise.
   */
  [[nodiscard]] virtual bool Exists(const std::string &name) const = 0;

  /**
   * @brief Computes checksums for the Porymap artifacts currently on disk.
   *
   * @param name The name of the Tileset for which to compute checksums.
   * @return A mapping of artifact identifiers to their computed checksum.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  ComputePorymapChecksums(const std::string &name) const = 0;

  /**
   * @brief Loads the persisted checksums from `artifact_checksums.json`.
   *
   * @param name The name of the Tileset for which to load checksums.
   * @return A mapping of artifact identifiers to their persisted checksum.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  LoadPersistedPorymapChecksums(const std::string &name) const = 0;
};
```

**Summary of Changes:**

1.  **`Save`**: The `Save` method's contract is clarified. It is now explicitly responsible for saving the `Tileset` and updating the checksums file atomically.
2.  **`LoadPersistedPorymapChecksums` (New Method)**: This new method provides the application layer with a way to read the last-saved checksums from `artifact_checksums.json` without loading the entire `Tileset` aggregate.

### 4. Use Case Implementation Example: `compile-tileset`

With the new repository interface, the application service for the `compile-tileset` use case would orchestrate the operation as follows:

```cpp
// Psuedocode for the compile-tileset application service
Result<void> handleCompileTileset(const std::string& tilesetName) {
  // 1. Check for unimported changes in Porymap assets, as per ARCHITECTURE.md.
  auto persistedChecksums = tilesetRepo->LoadPersistedPorymapChecksums(tilesetName);
  auto currentChecksums = tilesetRepo->ComputePorymapChecksums(tilesetName);

  if (persistedChecksums != currentChecksums) {
    return Error{"unimported changes present in Porymap assets, please run import-tileset first"};
  }

  // 2. Load the tileset aggregate.
  Result<std::unique_ptr<Tileset>> tilesetResult = tilesetRepo->Load(tilesetName);
  if (tilesetResult.IsErr()) {
    return tilesetResult.AsErr();
  }
  auto tileset = std::move(tilesetResult.AsVal());

  // 3. Perform the compilation. This is core domain logic.
  // This will update the PorymapTilesetComponent within the aggregate.
  Result<void> compilationResult = tileset->Compile();
  if (compilationResult.IsErr()) {
    return compilationResult;
  }

  // 4. Persist the updated tileset.
  // The Save implementation will handle writing the new binary assets
  // and updating artifact_checksums.json with the new checksums.
  return tilesetRepo->Save(*tileset);
}
```

## 5. Conclusion

This design successfully resolves the issue posed in the `TODO`:

-   It cleanly separates domain logic from persistence concerns.
-   The `Tileset` aggregate remains unpolluted by infrastructure details like checksums.
-   The `TilesetRepo` is given clear and cohesive responsibility for all persistence tasks.
-   The application layer correctly orchestrates the workflow, enforcing the rules specified in the use cases.

This approach is robust, maintainable, and aligns well with the Domain-Driven Design principles already established in the Porytiles2 architecture.
