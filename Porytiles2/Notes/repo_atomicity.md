# Tileset Repository Atomicity Strategies

## Problem Statement
The `TilesetRepo::save()` method in `tileset_repo.cpp` currently writes multiple artifacts sequentially. If any write fails partway through, we'd have a partially saved tileset which could corrupt the state on disk.

## Proposed Solutions

### 1. Transaction-Based Writer Interface
Modify `TilesetArtifactWriter` to support transactions:
- Add `begin_transaction()`, `commit()`, and `rollback()` methods
- Buffer all writes until commit
- On any failure, rollback removes/reverts all changes
- This pushes atomicity responsibility to the concrete implementation

**Pros:**
- Clean abstraction at interface level
- Different implementations can handle transactions appropriately
- Explicit transaction boundaries

**Cons:**
- Requires significant interface changes
- All implementations must support transactions
- May add complexity for simple use cases

### 2. Write-to-Staging Approach
Most practical for filesystem implementations:
- Write all artifacts to temporary locations (e.g., `.tmp/` subdirectory)
- Only after ALL writes succeed, atomically move files to final locations
- Use filesystem rename operations (generally atomic within same filesystem)
- On failure, simply delete staging directory

**Pros:**
- Simple and reliable for filesystem backing stores
- Naturally handles cleanup of partial writes
- Can validate all writes before committing

**Cons:**
- Filesystem-specific solution
- Requires temporary disk space
- May not work across filesystem boundaries

### 3. Batch Write Method
Add a batch write method to the writer interface:
```cpp
virtual Result<void> write_batch(
    const std::vector<std::tuple<std::any, TilesetArtifact, const Tileset&>>& operations
) = 0;
```
- Pass all write operations at once
- Implementation handles atomicity internally
- Filesystem impl could use temp files, database impl could use transactions

**Pros:**
- Single method call for atomic operations
- Implementation flexibility
- Clear intent in API

**Cons:**
- Requires interface changes
- May need to duplicate single-write logic

### 4. Unit of Work Pattern
Create a separate `TilesetSaveOperation` class:
- Accumulates all artifacts to write
- Validates everything upfront
- Executes as single atomic operation
- Can calculate checksums before committing

**Pros:**
- Encapsulates complexity in dedicated class
- Can add pre/post validation
- Reusable pattern for other operations

**Cons:**
- Additional abstraction layer
- More complex architecture

### 5. Implementation-Specific Strategies

#### For Filesystem Implementations
- Use directory renames (atomic on most filesystems)
- Write to `tileset_name.new/`, then rename to `tileset_name/`
- Keep backup as `tileset_name.old/` during transition

#### For Database Implementations
- Use native database transactions
- Much simpler - just BEGIN/COMMIT/ROLLBACK

## Recommended Approach

The **write-to-staging approach (#2)** combined with **batch write method (#3)** would be most flexible. 

### Implementation Strategy for Filesystem

1. **Staging Phase:**
   - Create staging directory with unique name (e.g., `tileset_name.tmp.{timestamp}`)
   - Write all artifacts to staging directory
   - Validate all writes completed successfully

2. **Commit Phase:**
   - Create backup of existing tileset (optional: `tileset_name.backup`)
   - Atomically rename/move files from staging to final locations
   - Or: rename entire directory if structure allows

3. **Cleanup Phase:**
   - Remove staging directory
   - Remove backup if commit succeeded
   - On failure, restore from backup if needed

### Example Pseudocode

```c++
Result<void> FilesystemWriter::write_batch(const WriteBatch& batch) {
    // Create staging area
    auto staging_dir = create_staging_directory();
    
    // Write all artifacts to staging
    for (const auto& [key, artifact, tileset] : batch) {
        auto staging_key = translate_to_staging(key, staging_dir);
        if (auto result = write_single(staging_key, artifact, tileset); !result) {
            cleanup_staging(staging_dir);
            return result;  // Propagate error
        }
    }
    
    // Atomic commit phase
    if (auto result = commit_staging_to_final(staging_dir); !result) {
        cleanup_staging(staging_dir);
        return result;
    }
    
    // Success - cleanup staging
    cleanup_staging(staging_dir);
    return {};
}
```

## Benefits of Recommended Approach

1. **Atomicity:** Either all artifacts are written or none are
2. **Consistency:** No partial states visible to readers
3. **Isolation:** Staging area isolates in-progress writes
4. **Durability:** Can verify writes before committing
5. **Rollback:** Easy to abort and cleanup on failure
6. **Stale Content Handling:** Naturally solves the TODO about clearing stale contents (line 26-27 in `tileset_repo.cpp`)

## Testing Considerations

1. **Unit Tests:**
   - Mock writer that simulates failures at different points
   - Verify no partial writes remain after failure
   - Test concurrent read/write scenarios

2. **Integration Tests:**
   - Test with actual filesystem
   - Simulate disk full scenarios
   - Test permission failures
   - Verify cleanup after crashes

## Migration Path

1. Start with current non-atomic implementation
2. Add batch write method to interface (optional at first)
3. Implement staging approach in concrete filesystem writer
4. Update `TilesetRepo::save()` to use batch write if available
5. Gradually migrate all writers to support atomicity