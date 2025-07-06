# Checksum Solution for TilesetRepo

## Problem Statement

The TODO comment in `TilesetRepo.hpp:24-31` raises a fundamental design question: how should we handle artifact checksums in a domain-driven design while supporting the use cases outlined in `ARCHITECTURE.md`?

The core tension is:
- **Domain Perspective**: Checksums feel like infrastructure/persistence details, not core domain logic
- **Use Case Requirements**: Checksums are essential for change detection between Porytiles and Porymap assets

## Analysis of Use Cases

From `ARCHITECTURE.md`, checksums are critical for:

1. **Import Primary Tileset** (lines 199-200): Compare checksums to detect "unimported changes present in Porymap asset X"
2. **Compile Primary Tileset** (lines 203-204): Compare checksums to detect "uncompiled changes in Porytiles asset X"  
3. **Create Primary Tileset** (lines 33-34): Generate initial checksums during tileset creation

## Proposed Solution: Domain Service Pattern

### Core Design Principles

1. **Separation of Concerns**: Keep checksum logic separate from the Tileset aggregate
2. **Template Method Pattern**: Use the suggested partial implementation approach
3. **Domain Service**: Create a dedicated service for checksum management
4. **Repository Collaboration**: Repository coordinates between domain and infrastructure

### Implementation Strategy

#### 1. Create ChecksumService Domain Service

```cpp
// porytiles2/domain/services/ChecksumService.hpp
class ChecksumService {
public:
    virtual ~ChecksumService() = default;
    
    [[nodiscard]] virtual std::unordered_map<std::string, std::string>
    ComputePorymapChecksums(const std::string& tileset_name) const = 0;
    
    [[nodiscard]] virtual std::unordered_map<std::string, std::string>
    LoadStoredChecksums(const std::string& tileset_name) const = 0;
    
    [[nodiscard]] virtual Result<void>
    StoreChecksums(const std::string& tileset_name, 
                   const std::unordered_map<std::string, std::string>& checksums) = 0;
};
```

#### 2. Implement Template Method in TilesetRepo

```cpp
// TilesetRepo.hpp
class TilesetRepo {
public:
    // Constructor injection of ChecksumService
    explicit TilesetRepo(std::unique_ptr<ChecksumService> checksum_service)
        : checksum_service_{std::move(checksum_service)} {}

    // Template method - partially implemented
    [[nodiscard]] Result<void> Save(const Tileset& tileset) {
        // 1. Delegate to concrete implementation
        auto save_result = SaveTileset(tileset);
        if (!save_result.IsOk()) {
            return save_result;
        }
        
        // 2. Handle checksum persistence (common behavior)
        auto current_checksums = checksum_service_->ComputePorymapChecksums(tileset.name());
        return checksum_service_->StoreChecksums(tileset.name(), current_checksums);
    }

    // Keep existing methods
    [[nodiscard]] virtual Result<std::unique_ptr<Tileset>> Load(const std::string& name) = 0;
    [[nodiscard]] virtual bool Exists(const std::string& name) const = 0;

protected:
    // Template method hook - concrete implementations override this
    [[nodiscard]] virtual Result<void> SaveTileset(const Tileset& tileset) = 0;
    
    // Provide access to checksum service for subclasses
    ChecksumService& checksum_service() { return *checksum_service_; }
    const ChecksumService& checksum_service() const { return *checksum_service_; }

private:
    std::unique_ptr<ChecksumService> checksum_service_;
};
```

#### 3. Create Application Service for Use Cases

```cpp
// porytiles2/app/services/TilesetCompilationService.hpp
class TilesetCompilationService {
public:
    TilesetCompilationService(std::unique_ptr<TilesetRepo> repo,
                             std::unique_ptr<ChecksumService> checksum_service)
        : repo_{std::move(repo)}, checksum_service_{std::move(checksum_service)} {}

    [[nodiscard]] Result<void> CompileTileset(const std::string& name) {
        // 1. Load tileset
        auto tileset_result = repo_->Load(name);
        if (!tileset_result.IsOk()) {
            return tileset_result.Error();
        }
        
        // 2. Check for unimported changes
        auto current_checksums = checksum_service_->ComputePorymapChecksums(name);
        auto stored_checksums = checksum_service_->LoadStoredChecksums(name);
        
        for (const auto& [artifact, current_sum] : current_checksums) {
            auto stored_it = stored_checksums.find(artifact);
            if (stored_it != stored_checksums.end() && stored_it->second != current_sum) {
                return Result<void>::Error("unimported changes present in Porymap asset " + artifact);
            }
        }
        
        // 3. Perform compilation logic...
        // 4. Save updated tileset (checksums updated automatically via template method)
        return repo_->Save(*tileset_result.Value());
    }

private:
    std::unique_ptr<TilesetRepo> repo_;
    std::unique_ptr<ChecksumService> checksum_service_;
};
```

## Benefits of This Approach

1. **Clean Domain Model**: Tileset aggregate remains focused on core domain logic
2. **Separation of Concerns**: Checksum logic is isolated in dedicated service
3. **Testability**: Each component can be unit tested independently
4. **Flexibility**: Different checksum implementations (file-based, database, etc.)
5. **Template Method**: Ensures checksum persistence happens automatically
6. **DDD Compliance**: Follows established patterns for infrastructure concerns

## Migration Strategy

1. **Phase 1**: Create ChecksumService interface and file-based implementation
2. **Phase 2**: Refactor TilesetRepo to use template method pattern
3. **Phase 3**: Update application services to use ChecksumService for change detection
4. **Phase 4**: Remove `ComputePorymapChecksums` from TilesetRepo interface

This solution addresses the TODO comment by providing a clean separation between domain logic and infrastructure concerns while maintaining all required functionality for the use cases.