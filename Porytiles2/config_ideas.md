# Configuration System Design

## Overview

This document outlines the design for a configuration system for Porytiles that follows domain-driven design and clean architecture principles. The system handles layered configuration with priority-based merging from CLI arguments, environment variables, TOML files, and default values.

## File Structure

```
Porytiles2/
├── include/porytiles2/
│   ├── domain/
│   │   ├── config/
│   │   │   ├── config.hpp
│   │   │   ├── fieldmap_settings.hpp
│   │   │   ├── palette_assignment_settings.hpp
│   │   │   └── compilation_settings.hpp
│   │   └── repos/
│   │       └── config_repository.hpp
│   ├── application/
│   │   └── services/
│   │       └── config_service.hpp
│   └── infrastructure/
│       └── config/
│           ├── config_repository_impl.hpp
│           ├── cli_config_source.hpp
│           ├── env_config_source.hpp
│           ├── toml_config_source.hpp
│           ├── config_layer_merger.hpp
│           └── partial_config.hpp
```

## Domain Layer

### Config (Aggregate Root)

```cpp
// include/porytiles2/domain/config/config.hpp
namespace porytiles2 {

/**
 * @brief Immutable aggregate root containing all application configuration.
 * 
 * @details
 * Config is constructed once at application startup and provides read-only
 * access to all configuration values. It owns value objects for related settings.
 */
class Config {
  public:
    // Constructor takes all value objects and primitives
    Config(FieldmapSettings fieldmap_settings,
           PaletteAssignmentSettings palette_settings,
           CompilationSettings compilation_settings,
           std::string output_path,
           int verbosity_level,
           bool enable_caching);

    // Read-only accessors
    [[nodiscard]] const FieldmapSettings &fieldmap_settings() const { return fieldmap_settings_; }
    [[nodiscard]] const PaletteAssignmentSettings &palette_settings() const { return palette_settings_; }
    [[nodiscard]] const CompilationSettings &compilation_settings() const { return compilation_settings_; }
    [[nodiscard]] const std::string &output_path() const { return output_path_; }
    [[nodiscard]] int verbosity_level() const { return verbosity_level_; }
    [[nodiscard]] bool enable_caching() const { return enable_caching_; }

  private:
    FieldmapSettings fieldmap_settings_;
    PaletteAssignmentSettings palette_settings_;
    CompilationSettings compilation_settings_;
    std::string output_path_;
    int verbosity_level_;
    bool enable_caching_;
};

} // namespace porytiles2
```

### Value Objects

```cpp
// include/porytiles2/domain/config/fieldmap_settings.hpp
namespace porytiles2 {

/**
 * @brief Value object containing fieldmap-related configuration.
 */
class FieldmapSettings {
  public:
    // Constructor validates that tile dimensions are positive
    FieldmapSettings(int tile_width, int tile_height, bool enable_dual_layer);

    [[nodiscard]] int tile_width() const { return tile_width_; }
    [[nodiscard]] int tile_height() const { return tile_height_; }
    [[nodiscard]] bool enable_dual_layer() const { return enable_dual_layer_; }

  private:
    int tile_width_;
    int tile_height_;
    bool enable_dual_layer_;
};

} // namespace porytiles2
```

```cpp
// include/porytiles2/domain/config/palette_assignment_settings.hpp
namespace porytiles2 {

/**
 * @brief Value object for palette assignment configuration.
 */
class PaletteAssignmentSettings {
  public:
    // Constructor validates palette count is within valid range
    PaletteAssignmentSettings(int max_palettes, 
                              bool auto_assign,
                              std::string assignment_algorithm);

    [[nodiscard]] int max_palettes() const { return max_palettes_; }
    [[nodiscard]] bool auto_assign() const { return auto_assign_; }
    [[nodiscard]] const std::string &assignment_algorithm() const { return assignment_algorithm_; }

  private:
    int max_palettes_;
    bool auto_assign_;
    std::string assignment_algorithm_;
};

} // namespace porytiles2
```

### Repository Interface

```cpp
// include/porytiles2/domain/repos/config_repository.hpp
namespace porytiles2 {

/**
 * @brief Port interface for loading configuration.
 */
class ConfigRepository {
  public:
    virtual ~ConfigRepository() = default;

    // Loads configuration from all sources and returns merged result
    [[nodiscard]] virtual Config load_config() = 0;
};

} // namespace porytiles2
```

## Application Layer

```cpp
// include/porytiles2/application/services/config_service.hpp
namespace porytiles2 {

/**
 * @brief Application service that manages configuration access.
 */
class ConfigService {
  public:
    // Inject the repository dependency
    explicit ConfigService(gsl::not_null<ConfigRepository *> config_repo);

    // Load configuration once at startup
    void initialize();

    // Get the loaded configuration (throws if not initialized)
    [[nodiscard]] const Config &config() const;

  private:
    gsl::not_null<ConfigRepository *> config_repo_;
    std::optional<Config> loaded_config_;
};

} // namespace porytiles2
```

## Infrastructure Layer

### Partial Configuration

```cpp
// include/porytiles2/infrastructure/config/partial_config.hpp
namespace porytiles2 {

/**
 * @brief Represents a partial configuration from a single source.
 * 
 * @details
 * Uses std::optional to represent values that may or may not be present
 * in a particular configuration source.
 */
struct PartialConfig {
    // Fieldmap settings
    std::optional<int> tile_width;
    std::optional<int> tile_height;
    std::optional<bool> enable_dual_layer;
    
    // Palette settings
    std::optional<int> max_palettes;
    std::optional<bool> auto_assign;
    std::optional<std::string> assignment_algorithm;
    
    // Compilation settings
    std::optional<int> optimization_level;
    std::optional<bool> generate_debug_info;
    
    // General settings
    std::optional<std::string> output_path;
    std::optional<int> verbosity_level;
    std::optional<bool> enable_caching;
};

} // namespace porytiles2
```

### Configuration Sources

```cpp
// include/porytiles2/infrastructure/config/cli_config_source.hpp
namespace porytiles2 {

/**
 * @brief Adapter for reading configuration from CLI arguments.
 */
class CliConfigSource {
  public:
    // Initialize with parsed CLI arguments
    explicit CliConfigSource(/* CLI parsing result type */);

    // Extract configuration values present in CLI args
    [[nodiscard]] PartialConfig read_config() const;

  private:
    // Stored parsed CLI arguments
};

} // namespace porytiles2
```

```cpp
// include/porytiles2/infrastructure/config/env_config_source.hpp
namespace porytiles2 {

/**
 * @brief Adapter for reading configuration from environment variables.
 */
class EnvConfigSource {
  public:
    // Define prefix for env vars (e.g., "PORYTILES_")
    explicit EnvConfigSource(std::string env_prefix = "PORYTILES_");

    // Read env vars and convert to partial config
    [[nodiscard]] PartialConfig read_config() const;

  private:
    std::string env_prefix_;
};

} // namespace porytiles2
```

```cpp
// include/porytiles2/infrastructure/config/toml_config_source.hpp
namespace porytiles2 {

/**
 * @brief Adapter for reading configuration from TOML file.
 */
class TomlConfigSource {
  public:
    // Initialize with path to TOML file
    explicit TomlConfigSource(std::filesystem::path toml_path);

    // Parse TOML file and return partial config
    [[nodiscard]] PartialConfig read_config() const;

  private:
    std::filesystem::path toml_path_;
};

} // namespace porytiles2
```

### Configuration Merging

```cpp
// include/porytiles2/infrastructure/config/config_layer_merger.hpp
namespace porytiles2 {

/**
 * @brief Merges multiple partial configs according to priority rules.
 */
class ConfigLayerMerger {
  public:
    // Define default values for all settings
    ConfigLayerMerger();

    // Merge configs in order of increasing priority
    [[nodiscard]] Config merge(const PartialConfig &defaults,
                               const PartialConfig &toml,
                               const PartialConfig &env,
                               const PartialConfig &cli) const;

  private:
    // Helper to pick value with highest priority
    template<typename T>
    [[nodiscard]] T select_value(const std::optional<T> &cli,
                                  const std::optional<T> &env,
                                  const std::optional<T> &toml,
                                  const T &default_val) const;
};

} // namespace porytiles2
```

### Repository Implementation

```cpp
// include/porytiles2/infrastructure/config/config_repository_impl.hpp
namespace porytiles2 {

/**
 * @brief Concrete implementation of ConfigRepository.
 */
class ConfigRepositoryImpl : public ConfigRepository {
  public:
    // Inject all configuration sources
    ConfigRepositoryImpl(std::unique_ptr<CliConfigSource> cli_source,
                         std::unique_ptr<EnvConfigSource> env_source,
                         std::unique_ptr<TomlConfigSource> toml_source,
                         std::unique_ptr<ConfigLayerMerger> merger);

    // Orchestrate reading from all sources and merging
    [[nodiscard]] Config load_config() override;

  private:
    std::unique_ptr<CliConfigSource> cli_source_;
    std::unique_ptr<EnvConfigSource> env_source_;
    std::unique_ptr<TomlConfigSource> toml_source_;
    std::unique_ptr<ConfigLayerMerger> merger_;
};

} // namespace porytiles2
```

## Key Design Decisions

1. **Config as Aggregate Root**: The `Config` class owns all settings and provides a unified, immutable interface.

2. **Value Objects**: Related settings are grouped into value objects with validation in constructors.

3. **Repository Pattern**: `ConfigRepository` interface in domain layer, implementation in infrastructure.

4. **Partial Configuration**: Infrastructure uses `PartialConfig` with `std::optional` to represent incomplete configs from each source.

5. **Clean Separation**: Domain knows nothing about CLI/env/TOML. Application orchestrates through the repository port. Infrastructure handles all external concerns.

6. **Layering Logic**: The `ConfigLayerMerger` encapsulates the priority rules, keeping this complexity in infrastructure.

## Configuration Priority

1. CLI arguments (highest priority)
2. Environment variables
3. TOML configuration file
4. Default values (lowest priority)

This design maintains clean architecture boundaries while providing the flexibility to add new configuration sources or change implementation details without affecting the domain model.