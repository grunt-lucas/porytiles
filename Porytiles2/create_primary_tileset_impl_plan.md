# Create Primary Tileset Implementation Plan

## Overview

This document outlines the implementation plan for the **Create Primary Tileset** use case in Porytiles2. This feature will create a new primary tileset with default assets and integrate it into the pokeemerald project structure.

## Use Case Summary

From `ARCHITECTURE.md`, the Create Primary Tileset use case:

### CLI Invocation
```sh
porytiles2 create-tileset MyTileset
```

### Logic Flow
1. Check if the primary tileset already exists. If so, abort with an error message.
2. Initialize a `PorytilesTilesetComponent` with default assets.
3. Compile the `PorytilesTilesetComponent` to generate an initial `PorymapTilesetComponent`.
4. Initialize a new `Tileset` aggregate with the components, while also generating the initial artifact checksums.
5. Persist the `Tileset` (including updating the right source and header files, i.e. `graphics.h`, `headers.h`, and `metatiles.h`).

## Current Architecture Analysis

### Existing Foundation
The Porytiles2 codebase provides excellent architectural foundation:

- **Domain Model**: Complete `Tileset` aggregate with `PorytilesTilesetComponent` and `PorymapTilesetComponent`
- **Repository Pattern**: `TilesetRepo` interface with `ProjectTilesetRepo` implementation
- **Use Case Framework**: `CompilePrimaryTileset` provides architectural template
- **CLI Infrastructure**: Command pattern with `Command` base class and CLI11 integration
- **Error Handling**: Robust `Result<T>` type (`std::expected<T, std::string>`)
- **Project Integration**: `ProjectPaths` class for pokeemerald file system operations

### Architecture Gaps
The following capabilities are **missing** and need to be implemented:

1. **C Source File Modification**: No existing infrastructure for modifying `.h` files
2. **Default Asset Generation**: No service for creating basic grass tiles and animated flowers
3. **Create Tileset Use Case**: No orchestration for the complete creation workflow
4. **CLI Create Command**: No command for tileset creation

## Implementation Strategy

### Phase 1: C Source File Modification Infrastructure

#### New Classes Required

**Domain Services (Interfaces)**
- `CSrcFileModifier` - Interface for modifying C header files
- `CSrcCodeGenerator` - Interface for generating C source code constructs

**Infrastructure Services (Implementations)**
- `PokeemeraldSrcFileModifier` - Concrete implementation for pokeemerald project files
- `TextBasedCSrcCodeGenerator` - Text-based C code generation (simpler than AST)
- `HeaderFileParser` - Service for parsing existing C header structure

#### Implementation Details

```cpp
// Domain service interface
class CSrcFileModifier {
public:
    virtual ~CSrcFileModifier() = default;
    virtual Result<void> AddTilesetDeclarations(const std::string& tileset_name) = 0;
    virtual Result<void> AddTilesetDefinition(const std::string& tileset_name) = 0;
    virtual Result<void> AddMetatileDeclarations(const std::string& tileset_name) = 0;
};

// Infrastructure implementation
class PokeemeraldSrcFileModifier : public CSrcFileModifier {
public:
    PokeemeraldSrcFileModifier(const ProjectPaths& paths);
    
    Result<void> AddTilesetDeclarations(const std::string& tileset_name) override;
    Result<void> AddTilesetDefinition(const std::string& tileset_name) override;
    Result<void> AddMetatileDeclarations(const std::string& tileset_name) override;
    
private:
    Result<void> ModifyGraphicsHeader(const std::string& tileset_name);
    Result<void> ModifyHeadersHeader(const std::string& tileset_name);
    Result<void> ModifyMetatilesHeader(const std::string& tileset_name);
    
    ProjectPaths paths_;
    std::unique_ptr<CSrcCodeGenerator> code_generator_;
};
```

**Files to Modify:**
- `src/data/tilesets/graphics.h` - Add palette and tile data declarations
- `src/data/tilesets/headers.h` - Add tileset struct definitions
- `src/data/tilesets/metatiles.h` - Add metatile and attribute array declarations

### Phase 2: Default Asset Generation

#### New Classes Required

**Domain Services**
- `DefaultAssetGenerator` - Interface for generating default tileset assets
- `DefaultAttributeGenerator` - Interface for generating CSV attributes

**Infrastructure Services**
- `BasicDefaultAssetGenerator` - Creates grass tiles and animated flowers
- `CsvDefaultAttributeGenerator` - Generates CSV with appropriate behaviors

#### Implementation Details

```cpp
class DefaultAssetGenerator {
public:
    virtual ~DefaultAssetGenerator() = default;
    virtual Result<PorytilesTilesetComponent> GenerateDefaultComponent() = 0;
};

class BasicDefaultAssetGenerator : public DefaultAssetGenerator {
public:
    Result<PorytilesTilesetComponent> GenerateDefaultComponent() override;
    
private:
    Result<std::vector<RgbaMetatile>> GenerateDefaultMetatiles();
    Result<std::vector<RgbaAnim>> GenerateDefaultAnimations();
    Result<RgbaImage> GenerateGrassTile();
    Result<RgbaAnim> GenerateFlowerAnimation();
};
```

**Default Asset Specifications:**
- **One row of metatiles** (8 metatiles, 128x16 pixels total)
- **Grass tiles**: Green variations with texture patterns
- **Animated flower**: Simple 2-3 frame animation
- **CSV attributes**: `MB_NORMAL` for walkable areas, `MB_TALL_GRASS` for grass
- **Layer structure**: `bottom.png`, `middle.png`, `top.png` with appropriate transparency

### Phase 3: Create Primary Tileset Use Case

#### New Classes Required

**Application Services**
- `CreatePrimaryTileset` - Main use case orchestrator
- `CreateTilesetCommand` - CLI command implementation

#### Implementation Details

```cpp
class CreatePrimaryTileset {
public:
    CreatePrimaryTileset(
        std::unique_ptr<TilesetRepo> tileset_repo,
        std::unique_ptr<DefaultAssetGenerator> asset_generator,
        std::unique_ptr<TilesetCompiler> compiler,
        std::unique_ptr<CSrcFileModifier> src_modifier,
        std::unique_ptr<ArtifactMetadataProvider> metadata_provider
    );
    
    Result<void> Execute(const std::string& tileset_name);
    
private:
    std::unique_ptr<TilesetRepo> tileset_repo_;
    std::unique_ptr<DefaultAssetGenerator> asset_generator_;
    std::unique_ptr<TilesetCompiler> compiler_;
    std::unique_ptr<CSrcFileModifier> src_modifier_;
    std::unique_ptr<ArtifactMetadataProvider> metadata_provider_;
};
```

**Workflow Implementation:**
1. **Existence Check**: Use `TilesetRepo::Exists()` to verify tileset doesn't exist
2. **Generate Defaults**: Use `DefaultAssetGenerator` to create `PorytilesTilesetComponent`
3. **Compile**: Use `TilesetCompiler::CompilePrimary()` to generate `PorymapTilesetComponent`
4. **Create Aggregate**: Initialize `Tileset` with both components
5. **Generate Checksums**: Use `ArtifactMetadataProvider` for initial checksums
6. **Persist Assets**: Use `TilesetRepo::Save()` to write tileset files
7. **Update C Files**: Use `CSrcFileModifier` to update pokeemerald source files

### Phase 4: CLI Integration

#### New Command Implementation

```cpp
class CreateTilesetCommand : public Command {
public:
    CreateTilesetCommand();
    
    int Execute() override;
    
private:
    std::string tileset_name_;
    std::unique_ptr<CreatePrimaryTileset> use_case_;
};
```

**CLI Integration Points:**
- Add to command registry in driver
- Handle command-line argument parsing
- Provide user feedback and error messages
- Follow existing command patterns

## Technical Considerations

### Text-Based C File Modification

**Rationale**: Using text-based parsing instead of clang AST manipulation:
- **Simpler implementation** - No clang dependency
- **Faster to develop** - String manipulation vs AST traversal
- **Sufficient for use case** - Adding declarations at specific locations
- **Easier to debug** - Human-readable text operations

**Approach**:
- Parse C files line-by-line to find insertion points
- Generate C code using string templates
- Handle proper indentation and formatting
- Validate syntax with simple regex patterns

### Error Handling Strategy

Follow existing `Result<T>` pattern throughout:
- File I/O operations return `Result<void>` or `Result<T>`
- Chain operations using monadic composition
- Provide detailed error messages for user feedback
- Rollback capability for partial failures

### Directory Structure

**Generated Assets Location:**
```
data/tilesets/primary/my_tileset/
├── metatile_attributes.bin
├── metatiles.bin
├── tiles.png
├── anim/
│   └── flower/
│       ├── 00.png
│       ├── 01.png
│       └── 02.png
├── palettes/
│   ├── 00.pal
│   └── ...
└── porytiles/
    ├── bottom.png
    ├── middle.png
    ├── top.png
    ├── attributes.csv
    ├── my_tileset.toml
    ├── artifact_checksums.json
    └── anim/
        └── flower/
            ├── key.png
            ├── 00.png
            ├── 01.png
            └── 02.png
```

### Integration Points

**Repository Layer**:
- Extend `ProjectTilesetRepo` to support creation operations
- Add validation for tileset name conflicts
- Handle atomic operations (create all files or none)

**Compilation Layer**:
- Use existing `TilesetCompiler` interface
- Leverage `CompilePrimaryTileset` use case patterns
- Generate initial checksums for change detection

**CLI Layer**:
- Add command to existing driver program
- Follow established option parsing patterns
- Provide progress feedback during creation

## Development Phases

### Phase 1: Foundation (Week 1-2)
- Implement `CSrcFileModifier` and related services
- Create text-based C code generation utilities
- Add unit tests for C file modification

### Phase 2: Asset Generation (Week 2-3)
- Implement `DefaultAssetGenerator` services
- Create grass tile and flower animation generators
- Add CSV attribute generation capabilities

### Phase 3: Use Case Integration (Week 3-4)
- Implement `CreatePrimaryTileset` use case
- Add CLI command implementation
- Integration testing with existing components

### Phase 4: Polish and Testing (Week 4-5)
- Comprehensive testing with pokeemerald projects
- Error handling and edge case validation
- Documentation and examples

## Success Criteria

- [ ] Can create new primary tileset with single CLI command
- [ ] Generates functional default assets (grass + animated flower)
- [ ] Properly integrates with pokeemerald project structure
- [ ] Updates all required C header files correctly
- [ ] Follows existing code style and architecture patterns
- [ ] Provides clear error messages for failure cases
- [ ] Includes comprehensive unit and integration tests

## Next Steps

1. **Start with Phase 1** - Focus on C file modification infrastructure
2. **Create prototypes** - Test C code generation with simple examples
3. **Iterate on design** - Refine interfaces based on implementation experience
4. **Add comprehensive tests** - Ensure reliability with various pokeemerald projects
5. **Document usage** - Create examples and tutorials for end users

This implementation plan provides a structured approach to adding the Create Primary Tileset feature while leveraging the existing Porytiles2 architecture and maintaining consistency with established patterns.