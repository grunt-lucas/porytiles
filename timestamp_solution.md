# Timestamp Service Design for Porytiles2

## Overview

This document outlines the design for a timestamp service that will handle asset modification time tracking and comparison between Porymap and Porytiles assets. The design follows domain-driven principles consistent with the existing ChecksumService architecture.

## Problem Statement

The `CompilePrimaryTileset` use case needs to check if the newest Porymap asset "modified" timestamp is newer than the newest Porytiles asset "modified" timestamp. If so, it should bail with "nothing to do" to avoid unnecessary recompilation.

## Domain-Driven Design Architecture

### 1. TimestampService Interface

Following the same pattern as `ChecksumService`, create a pure virtual interface:

```cpp
// Porytiles2/include/porytiles2/domain/services/TimestampService.hpp
#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

using Timestamp = std::chrono::file_time_type;

class TimestampService {
public:
  virtual ~TimestampService() = default;

  /**
   * @brief Gets the modification timestamps for all Porymap artifacts associated with the given Tileset.
   *
   * @param tileset The Tileset for which to get Porymap artifact timestamps.
   * @return A mapping of artifact identifiers to their modification timestamps.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
  GetPorymapTimestamps(const Tileset &tileset) const = 0;

  /**
   * @brief Gets the modification timestamps for all Porytiles artifacts associated with the given Tileset.
   *
   * @param tileset The Tileset for which to get Porytiles artifact timestamps.
   * @return A mapping of artifact identifiers to their modification timestamps.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
  GetPorytilesTimestamps(const Tileset &tileset) const = 0;

  /**
   * @brief Determines if any Porymap assets are newer than any Porytiles assets.
   *
   * @param tileset The Tileset to check.
   * @return True if Porymap assets are newer, false otherwise.
   */
  [[nodiscard]] virtual bool ArePorymapAssetsNewer(const Tileset &tileset) const = 0;
};

} // namespace porytiles
```

### 2. Integration with TilesetRepo

The `TilesetRepo` should be enhanced to support both checksum and timestamp services:

```cpp
// Modified TilesetRepo constructor and member
class TilesetRepo {
public:
  explicit TilesetRepo(std::unique_ptr<ChecksumService> checksum_service,
                       std::unique_ptr<TimestampService> timestamp_service)
      : checksum_service_{std::move(checksum_service)},
        timestamp_service_{std::move(timestamp_service)} {}

protected:
  [[nodiscard]] TimestampService &timestamp_service() { return *timestamp_service_; }
  [[nodiscard]] const TimestampService &timestamp_service() const { return *timestamp_service_; }

private:
  std::unique_ptr<ChecksumService> checksum_service_;
  std::unique_ptr<TimestampService> timestamp_service_;
};
```

### 3. Application Layer Integration

The `CompilePrimaryTileset` use case should be updated to use the timestamp service:

```cpp
// Modified CompilePrimaryTileset::Compile method
Result<void> CompilePrimaryTileset::Compile(const std::string &tileset_name) const {
  // 1. Load tileset
  auto maybe_tileset = tileset_repo_->Load(tileset_name);
  if (!maybe_tileset.has_value()) {
    return std::unexpected{maybe_tileset.error()};
  }
  const auto tileset = std::move(maybe_tileset.value());

  // 2. Check for unimported changes
  auto current_checksums = checksum_service_->ComputePorymapChecksums(*tileset);
  auto stored_checksums = checksum_service_->LoadStoredChecksums(tileset_name);

  for (const auto &[artifact, current_sum] : current_checksums) {
    if (auto stored_it = stored_checksums.find(artifact);
        stored_it != stored_checksums.end() && stored_it->second != current_sum) {
      return std::unexpected{"unimported changes present in Porymap asset " + artifact};
    }
  }

  // 3. Check if Porymap assets are newer than Porytiles assets
  if (timestamp_service_->ArePorymapAssetsNewer(*tileset)) {
    return std::unexpected{"nothing to do - Porymap assets are newer than Porytiles assets"};
  }

  // 4. Continue with compilation logic...
  // Rest of the method remains the same
}
```

## Implementation Strategy

### 1. Concrete Implementation

Create a filesystem-based implementation similar to how `ProjectTilesetRepo` implements `TilesetRepo`:

```cpp
// Porytiles2/include/porytiles2/infra/services/FilesystemTimestampService.hpp
#pragma once

#include "porytiles2/domain/services/TimestampService.hpp"

namespace porytiles {

class FilesystemTimestampService : public TimestampService {
public:
  // Constructor takes base paths for project structure
  explicit FilesystemTimestampService(const std::string &project_root);

  [[nodiscard]] std::unordered_map<std::string, Timestamp>
  GetPorymapTimestamps(const Tileset &tileset) const override;

  [[nodiscard]] std::unordered_map<std::string, Timestamp>
  GetPorytilesTimestamps(const Tileset &tileset) const override;

  [[nodiscard]] bool ArePorymapAssetsNewer(const Tileset &tileset) const override;

private:
  std::string project_root_;
  
  // Helper methods to get file paths for different asset types
  [[nodiscard]] std::vector<std::string> GetPorymapArtifactPaths(const Tileset &tileset) const;
  [[nodiscard]] std::vector<std::string> GetPorytilesArtifactPaths(const Tileset &tileset) const;
  [[nodiscard]] Timestamp GetFileTimestamp(const std::string &filepath) const;
};

} // namespace porytiles
```

### 2. Dependency Injection

Update the application's composition root to inject the timestamp service:

```cpp
// In the main application setup
auto checksum_service = std::make_unique<ConcreteChecksumService>();
auto timestamp_service = std::make_unique<FilesystemTimestampService>(project_root);
auto tileset_repo = std::make_unique<ProjectTilesetRepo>(
    std::move(checksum_service), 
    std::move(timestamp_service)
);
auto compile_use_case = std::make_unique<CompilePrimaryTileset>(
    std::move(tileset_repo), 
    compiler_service
);
```

## Design Principles Followed

### 1. Domain-Driven Design
- **Service Pattern**: `TimestampService` is a domain service that encapsulates timestamp-related business logic
- **Aggregate Root**: Operations are performed on the `Tileset` aggregate root
- **Repository Pattern**: Service is injected into the repository following the same pattern as `ChecksumService`

### 2. Dependency Inversion
- Abstract interface defines the contract
- Concrete implementations handle infrastructure concerns
- Application layer depends on abstractions, not concretions

### 3. Single Responsibility
- `TimestampService` is only responsible for timestamp operations
- Separate from checksum concerns while following the same architectural pattern
- Each method has a single, well-defined purpose

### 4. Consistency with Existing Architecture
- Follows the same naming conventions as `ChecksumService`
- Uses the same error handling pattern with `Result<T>`
- Integrates seamlessly with the existing repository pattern

## Benefits

1. **Maintainability**: Clear separation of concerns with timestamp logic isolated in its own service
2. **Testability**: Interface allows for easy mocking in unit tests
3. **Extensibility**: New timestamp-related features can be added to the service interface
4. **Consistency**: Follows the same architectural patterns as the existing checksum service
5. **Performance**: Avoids unnecessary compilation when source assets haven't changed

## Testing Strategy

- Unit tests for the service interface using mocked filesystem operations
- Integration tests with real filesystem timestamps
- Test boundary conditions (missing files, permission issues, etc.)
- Test the integration with the `CompilePrimaryTileset` use case

This design maintains the clean architecture principles while providing the necessary functionality to optimize compilation by checking asset timestamps.