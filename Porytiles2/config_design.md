# Configuration System Design

This document outlines the design for a flexible, layered configuration system for Porytiles following domain-driven design and clean architecture principles.

## Feedback and Questions from Code Review

### Overall Design Assessment
The design follows DDD principles well with clear separation of concerns. However, several areas need clarification:

1. **Partial Configuration Concept**: What exactly is a "partial configuration"? How do we represent settings that are unset vs explicitly set to a value?
2. **Merge Strategy**: The merge logic needs more detail. What happens when merging configurations with conflicting values?
3. **Validation Strategy**: Where and how should validation errors be handled? Should value objects throw exceptions in constructors?
4. **Configuration Evolution**: How do we handle adding new settings or changing existing ones over time?

### Specific Concerns

#### Domain Layer
- **IConfigurationRepository placement**: Should this interface really be in the domain layer? It seems more like an application/infrastructure concern since the domain shouldn't know about persistence details like TOML.
- **Configuration Construction**: The class has both a default constructor and a static factory. How do these work together? Consider making the default constructor private.
- **Missing Includes**: Headers are missing necessary includes (e.g., `<optional>`, `<string>`, `<filesystem>`, `<memory>`)

#### Application Layer
- **Error Handling**: How should the ConfigurationService handle errors from parsing/reading? Should it throw, return Result types, or use another pattern?
- **Canonical TOML Path**: What exactly is the "canonical location" for the TOML file? Should this be configurable?

#### Infrastructure Layer
- **parsed_args Type**: The `parsed_args` type in CliConfigurationParser is undefined. What library will you use?
- **Environment Variable Convention**: What's the naming convention for environment variables? The example shows `PORYTILES_FIELDMAP_WIDTH` - is this the pattern?

### Questions Requiring Clarification

1. **Optional vs Required Settings**: How do we distinguish between optional and required configuration values? 
2. **Default Values**: Where should default values be defined? In the domain objects themselves or centralized in ConfigurationService?
3. **Thread Safety**: Is thread safety a concern? The design shows const methods but uses unique_ptr members.
4. **TOML Schema**: What will the TOML file structure look like? Should we document the schema?
5. **Extensibility**: How easy will it be to add new configuration sources (e.g., JSON files, remote config)?

### Suggested Enhancements

Consider adding:
- A `ConfigurationBuilder` class for more flexible construction
- A `PartialConfiguration` wrapper type to distinguish set vs unset values
- Error types for configuration-related failures
- A schema/documentation generator for the TOML format

### Summary of Key Design Decisions Needed

Before implementing, please clarify:
1. **Partial Configuration Strategy**: How to represent unset vs default values
2. **Error Handling**: Exceptions vs Result types vs another approach
3. **Repository Placement**: Move IConfigurationRepository to application layer?
4. **Default Values**: Where defaults should be defined (domain vs application)
5. **TOML Path**: Make the TOML file path optional or configurable
6. **CLI Library Choice**: Which library for argument parsing (affects parsed_args type)

---

## Requirements

The system is based on a core aggregate root called `Configuration` containing all config settings as private members. For intrinsically related settings requiring internal validation, pure value-object containers like `FieldmapSettings` or `PaletteAssignmentSettings` are used.

### Configuration Layering Priority
1. CLI driver override options (highest precedence)
2. Environment variable configuration
3. TOML file at canonical location
4. Default values (lowest precedence)

### Key Constraints
- Configuration is read-only (Porytiles is a one-shot CLI program)
- Exception: Must support writing initial TOML configuration files for new tilesets

## File Structure
```
Porytiles2/
├── include/porytiles2/
│   ├── domain/
│   │   └── configuration/
│   │       ├── Configuration.hpp
│   │       ├── FieldmapSettings.hpp
│   │       ├── PaletteAssignmentSettings.hpp
│   │       └── IConfigurationRepository.hpp
│   ├── application/
│   │   └── configuration/
│   │       ├── ConfigurationService.hpp
│   │       ├── ICliConfigurationParser.hpp
│   │       └── IEnvironmentVariableReader.hpp
│   └── infrastructure/
│       └── configuration/
│           ├── TomlConfigurationRepository.hpp
│           ├── CliConfigurationParser.hpp
│           └── EnvironmentVariableReader.hpp
└── lib/ (corresponding .cpp files)
```

## Domain Layer Classes

### Configuration.hpp (Aggregate Root)
```cpp
// FEEDBACK: Missing required includes
#include <string>
#include "porytiles2/domain/configuration/FieldmapSettings.hpp"
#include "porytiles2/domain/configuration/PaletteAssignmentSettings.hpp"

namespace porytiles2 {

class Configuration {
public:
    // FEEDBACK: Consider making this private and friend the ConfigurationService
    // to enforce construction through proper channels
    Configuration() = default;
    
    // Factory method to create with all settings
    static Configuration create(
        const FieldmapSettings& fieldmap_settings,
        const PaletteAssignmentSettings& palette_settings,
        int some_other_setting  // FEEDBACK: Replace with actual domain-specific type
    );
    
    // Read-only accessors
    [[nodiscard]] const FieldmapSettings& fieldmap_settings() const;
    [[nodiscard]] const PaletteAssignmentSettings& palette_settings() const;
    [[nodiscard]] int some_other_setting() const;

    // FEEDBACK: Consider adding these methods for better usability:
    // [[nodiscard]] bool has_fieldmap_settings() const;
    // [[nodiscard]] bool has_palette_settings() const;
    
private:
    // FEEDBACK: How do we represent "not set" vs "set to default"?
    // Consider using std::optional<FieldmapSettings> for partial configs
    FieldmapSettings fieldmap_settings_;
    PaletteAssignmentSettings palette_settings_;
    int some_other_setting_{0};
};

}
```

### FieldmapSettings.hpp (Value Object)
```cpp
namespace porytiles2 {

class FieldmapSettings {
public:
    // Constructor validates that width and height are reasonable
    FieldmapSettings(int width, int height, bool enable_transparency);
    
    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;
    [[nodiscard]] bool enable_transparency() const;
    
    // Value object equality
    bool operator==(const FieldmapSettings& other) const = default;

private:
    int width_;
    int height_;
    bool enable_transparency_;
};

}
```

### PaletteAssignmentSettings.hpp (Value Object)
```cpp
namespace porytiles2 {

class PaletteAssignmentSettings {
public:
    // Constructor validates palette count and algorithm choice
    PaletteAssignmentSettings(int max_palettes, std::string algorithm);
    
    [[nodiscard]] int max_palettes() const;
    [[nodiscard]] const std::string& algorithm() const;
    
    bool operator==(const PaletteAssignmentSettings& other) const = default;

private:
    int max_palettes_;
    std::string algorithm_;
};

}
```

### IConfigurationRepository.hpp (Domain Interface)
```cpp
// FEEDBACK: Missing includes
#include <optional>
#include <filesystem>
#include "porytiles2/domain/configuration/Configuration.hpp"

namespace porytiles2 {

// FEEDBACK: This interface seems too specific to TOML implementation details.
// Consider a more generic interface like IConfigurationPersistence or moving
// this to the application layer since the domain shouldn't know about file formats
class IConfigurationRepository {
public:
    virtual ~IConfigurationRepository() = default;
    
    // FEEDBACK: "partial configuration" concept needs definition
    // Consider creating a PartialConfiguration type or using a different approach
    // Load partial configuration from TOML file, returns nullopt if file doesn't exist
    virtual std::optional<Configuration> load_from_toml(const std::filesystem::path& path) const = 0;
    
    // Save configuration to TOML file for initial project setup
    virtual void save_to_toml(const Configuration& config, const std::filesystem::path& path) const = 0;
    
    // FEEDBACK: Consider error handling - what if save fails?
    // Maybe return a Result<void, ConfigurationError> or throw?
};

}
```

## Application Layer Classes

### ConfigurationService.hpp
```cpp
// FEEDBACK: Missing includes
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <optional>
#include "porytiles2/domain/configuration/Configuration.hpp"
#include "porytiles2/domain/configuration/IConfigurationRepository.hpp"
#include "porytiles2/application/configuration/ICliConfigurationParser.hpp"
#include "porytiles2/application/configuration/IEnvironmentVariableReader.hpp"

namespace porytiles2 {

class ConfigurationService {
public:
    ConfigurationService(
        std::unique_ptr<IConfigurationRepository> toml_repo,
        std::unique_ptr<ICliConfigurationParser> cli_parser,
        std::unique_ptr<IEnvironmentVariableReader> env_reader
    );
    
    // FEEDBACK: What happens if toml_path doesn't exist? Should it be optional?
    // Consider returning Result<Configuration, ConfigurationError> for better error handling
    // Load configuration with proper layering: CLI > Env > TOML > Defaults
    [[nodiscard]] Configuration load_configuration(
        const std::vector<std::string>& cli_args,
        const std::filesystem::path& toml_path  // FEEDBACK: Make this optional?
    ) const;
    
    // FEEDBACK: Should this use a template/default configuration?
    // Create initial TOML file with default settings
    void create_initial_toml_file(const std::filesystem::path& path) const;

private:
    std::unique_ptr<IConfigurationRepository> toml_repo_;
    std::unique_ptr<ICliConfigurationParser> cli_parser_;
    std::unique_ptr<IEnvironmentVariableReader> env_reader_;
    
    // FEEDBACK: How does merge work? Need clear precedence rules documented
    // What if cli_config has fieldmap settings but not palette settings?
    // Apply layering logic by merging partial configurations
    Configuration merge_configurations(
        const std::optional<Configuration>& cli_config,
        const std::optional<Configuration>& env_config,
        const std::optional<Configuration>& toml_config
    ) const;
    
    // FEEDBACK: Should defaults be hardcoded here or come from domain objects?
    // Get default configuration values
    Configuration get_default_configuration() const;
};

}
```

### ICliConfigurationParser.hpp
```cpp
namespace porytiles2 {

class ICliConfigurationParser {
public:
    virtual ~ICliConfigurationParser() = default;
    
    // Parse CLI arguments and return partial configuration (only overridden values)
    virtual std::optional<Configuration> parse_cli_arguments(
        const std::vector<std::string>& args
    ) const = 0;
};

}
```

### IEnvironmentVariableReader.hpp
```cpp
namespace porytiles2 {

class IEnvironmentVariableReader {
public:
    virtual ~IEnvironmentVariableReader() = default;
    
    // Read environment variables and return partial configuration
    virtual std::optional<Configuration> read_environment_variables() const = 0;
};

}
```

## Infrastructure Layer Classes

### TomlConfigurationRepository.hpp
```cpp
namespace porytiles2 {

class TomlConfigurationRepository : public IConfigurationRepository {
public:
    TomlConfigurationRepository() = default;
    
    // Parse TOML file using a library like toml11
    std::optional<Configuration> load_from_toml(const std::filesystem::path& path) const override;
    
    // Serialize Configuration to TOML format
    void save_to_toml(const Configuration& config, const std::filesystem::path& path) const override;

private:
    // Helper methods for TOML parsing/serialization
    FieldmapSettings parse_fieldmap_settings(const toml::value& toml_data) const;
    PaletteAssignmentSettings parse_palette_settings(const toml::value& toml_data) const;
};

}
```

### CliConfigurationParser.hpp
```cpp
namespace porytiles2 {

class CliConfigurationParser : public ICliConfigurationParser {
public:
    CliConfigurationParser() = default;
    
    // Use a library like CLI11 or argparse to parse command line arguments
    std::optional<Configuration> parse_cli_arguments(
        const std::vector<std::string>& args
    ) const override;

private:
    // Helper methods for specific argument parsing
    std::optional<FieldmapSettings> parse_fieldmap_args(const parsed_args& args) const;
    std::optional<PaletteAssignmentSettings> parse_palette_args(const parsed_args& args) const;
};

}
```

### EnvironmentVariableReader.hpp
```cpp
namespace porytiles2 {

class EnvironmentVariableReader : public IEnvironmentVariableReader {
public:
    EnvironmentVariableReader() = default;
    
    // Read from environment variables like PORYTILES_FIELDMAP_WIDTH, etc.
    std::optional<Configuration> read_environment_variables() const override;

private:
    // Helper methods for reading specific environment variable groups
    std::optional<FieldmapSettings> read_fieldmap_env_vars() const;
    std::optional<PaletteAssignmentSettings> read_palette_env_vars() const;
    std::optional<std::string> get_env_var(const std::string& name) const;
};

}
```

## Key Design Benefits

1. **DDD Compliance**: Clear aggregate root, value objects, and domain interfaces
2. **Dependency Inversion**: Domain interfaces implemented in infrastructure
3. **Single Responsibility**: Each class has one clear purpose
4. **Extensibility**: Easy to add new configuration sources or settings
5. **Testability**: All dependencies can be mocked via interfaces
6. **Immutability**: Configuration is read-only after construction
7. **Layering Support**: ConfigurationService handles precedence logic cleanly

The design separates concerns properly while maintaining flexibility for future extensions.

## Alternative Design Suggestions

### 1. Partial Configuration Representation
Instead of using `std::optional<Configuration>`, consider a dedicated type:

```cpp
template<typename T>
struct ConfigValue {
    bool is_set;
    T value;
    
    static ConfigValue<T> unset() { return {false, T{}}; }
    static ConfigValue<T> of(T val) { return {true, std::move(val)}; }
};

struct PartialConfiguration {
    ConfigValue<FieldmapSettings> fieldmap_settings;
    ConfigValue<PaletteAssignmentSettings> palette_settings;
    // etc...
};
```

### 2. Builder Pattern for Configuration
```cpp
class ConfigurationBuilder {
public:
    ConfigurationBuilder& with_fieldmap_settings(const FieldmapSettings& settings);
    ConfigurationBuilder& with_palette_settings(const PaletteAssignmentSettings& settings);
    ConfigurationBuilder& apply_partial(const PartialConfiguration& partial);
    [[nodiscard]] Configuration build() const;
    
private:
    // Track what's been set
    std::optional<FieldmapSettings> fieldmap_;
    std::optional<PaletteAssignmentSettings> palette_;
};
```

### 3. Configuration Schema Documentation
Consider adding a schema definition that can generate both documentation and validation:

```cpp
// In domain layer
struct ConfigurationSchema {
    struct Field {
        std::string name;
        std::string type;
        std::string description;
        bool required;
        std::string default_value;
        std::string env_var_name;
        std::string cli_flag;
    };
    
    static std::vector<Field> get_schema();
};
```

### 4. Error Handling Strategy
Define clear error types:

```cpp
enum class ConfigurationErrorKind {
    InvalidValue,
    MissingRequired,
    ParseError,
    IOError
};

struct ConfigurationError {
    ConfigurationErrorKind kind;
    std::string message;
    std::optional<std::string> field_name;
};

// Use Result<T, ConfigurationError> for return types
```

### 5. TOML File Example
Document the expected TOML structure:

```toml
# porytiles.toml
[fieldmap]
width = 32
height = 32
enable_transparency = true

[palette_assignment]
max_palettes = 6
algorithm = "best_fit"

# Environment variable equivalents:
# PORYTILES_FIELDMAP_WIDTH=32
# PORYTILES_FIELDMAP_HEIGHT=32
# PORYTILES_FIELDMAP_ENABLE_TRANSPARENCY=true
# PORYTILES_PALETTE_ASSIGNMENT_MAX_PALETTES=6
# PORYTILES_PALETTE_ASSIGNMENT_ALGORITHM=best_fit
```