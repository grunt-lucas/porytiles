# A `std::any` Solution
```c++
#include <any>
#include <string>
#include <vector>
#include <filesystem> // For an example implementation

// Dummy types for the example
struct VramMetatile {};
struct Attributes {};
class Tileset {
  public:
    const std::string& name() const { return name_; }
    // porymap_component().metatiles() and porymap_component().attributes() assumed to exist
  private:
    std::string name_{"example_tileset"};
};


// --- INTERFACES ---

class IKeyProvider {
  public:
    virtual ~IKeyProvider() = default;
    virtual std::any porymap_metatiles(const std::string &tileset_name) const = 0;
    virtual std::any porymap_attributes(const std::string &tileset_name) const = 0;
    virtual bool exists(const std::any &key) const = 0;
};

class IWriter {
  public:
    virtual ~IWriter() = default;
    virtual void write(const std::any &key, const std::vector<VramMetatile> &metatiles) = 0;
    virtual void write(const std::any &key, const std::vector<Attributes> &attr) = 0;
};
```
```c++
// --- CONCRETE IMPLEMENTATIONS ---

class FilePathKeyProvider : public IKeyProvider {
  public:
    std::any porymap_metatiles(const std::string &tileset_name) const override {
        return std::filesystem::path{tileset_name + "/metatiles.bin"};
    }

    std::any porymap_attributes(const std::string &tileset_name) const override {
        return std::filesystem::path{tileset_name + "/attributes.bin"};
    }

    bool exists(const std::any &key) const override {
        // Cast the 'any' to the expected type. Throws std::bad_any_cast if types mismatch.
        return std::filesystem::exists(std::any_cast<std::filesystem::path>(key));
    }
};

class FileWriter : public IWriter {
  public:
    void write(const std::any &key, const std::vector<VramMetatile> &metatiles) override {
        const auto path = std::any_cast<std::filesystem::path>(key);
        // ... logic to write metatiles to file at 'path'
        std::cout << "Writing metatiles to: " << path << std::endl;
    }

    void write(const std::any &key, const std::vector<Attributes> &attr) override {
        const auto path = std::any_cast<std::filesystem::path>(key);
        // ... logic to write attributes to file at 'path'
        std::cout << "Writing attributes to: " << path << std::endl;
    }
};
```

```c++
class TilesetRepo {
  public:
    // Dependencies are injected via references to the abstract interfaces.
    TilesetRepo(const IKeyProvider &provider, IWriter &writer)
        : key_provider_{provider}, writer_{writer} {}

    void save_tileset(const Tileset &tileset) {
        // The repo doesn't know or care what type the key is.
        // It just passes the std::any from the provider to the writer.
        auto metatiles_key = key_provider_.porymap_metatiles(tileset.name());
        auto attr_key = key_provider_.porymap_attributes(tileset.name());

        if (key_provider_.exists(metatiles_key)) {
            std::cout << "Key exists, overwriting..." << std::endl;
        }

        // Dummy data for the example
        std::vector<VramMetatile> metatiles_data;
        std::vector<Attributes> attributes_data;
        
        writer_.write(metatiles_key, metatiles_data);
        writer_.write(attr_key, attributes_data);
    }

  private:
    const IKeyProvider &key_provider_;
    IWriter &writer_;
};
```

