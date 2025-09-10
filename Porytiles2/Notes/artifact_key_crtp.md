# CRTP Solution for Virtual Template Functions in ArtifactKey

## Problem Statement

The following code is **not legal C++** because virtual functions cannot be templated:

```cpp
template<typename T>
[[nodiscard]] virtual ArtifactKey<T> key_for(const std::string &tileset_name, const TilesetArtifact &artifact) const = 0;
```

The compiler needs to know all virtual function signatures at compile time to build the vtable, but template instantiations aren't known until they're used.

## Solution: CRTP (Curiously Recurring Template Pattern)

Move the template to the class level and use static polymorphism instead of virtual functions.

### Basic Implementation

```cpp
template<typename T, typename Derived>
class ArtifactKeyProvider {
public:
    [[nodiscard]] ArtifactKey<T> key_for(const std::string &tileset_name, 
                                         const TilesetArtifact &artifact) const {
        return static_cast<const Derived*>(this)->key_for_impl(tileset_name, artifact);
    }
};

template<typename T>
class ConcreteProvider : public ArtifactKeyProvider<T, ConcreteProvider<T>> {
public:
    [[nodiscard]] ArtifactKey<T> key_for_impl(const std::string &tileset_name,
                                              const TilesetArtifact &artifact) const {
        // Implementation
        return ArtifactKey<T>{/* ... */};
    }
};
```

## Enhanced Version with C++20 Concepts

### Simple Concept Constraint

```cpp
#include <concepts>
#include <type_traits>

// Concept to check if Derived has the correct key_for_impl method
template<typename Derived, typename T>
concept HasKeyForImpl = requires(const Derived& d, 
                                 const std::string& name, 
                                 const TilesetArtifact& artifact) {
    { d.key_for_impl(name, artifact) } -> std::same_as<ArtifactKey<T>>;
};

// CRTP base with concept constraint
template<typename T, typename Derived>
    requires HasKeyForImpl<Derived, T>
class ArtifactKeyProvider {
public:
    [[nodiscard]] ArtifactKey<T> key_for(const std::string& tileset_name, 
                                         const TilesetArtifact& artifact) const {
        return static_cast<const Derived*>(this)->key_for_impl(tileset_name, artifact);
    }
};
```

### Comprehensive Concept with Additional Checks

```cpp
// More detailed concept with additional checks
template<typename Derived, typename T>
concept HasKeyForImpl = requires {
    // Check that Derived is a complete type
    sizeof(Derived);
} && requires(const Derived& d, 
              const std::string& name, 
              const TilesetArtifact& artifact) {
    // Check the method exists and returns the correct type
    { d.key_for_impl(name, artifact) } -> std::same_as<ArtifactKey<T>>;
    
    // Ensure it's const-qualified (this is checked by using const Derived&)
};

// You could also add a concept for the overall class requirements
template<typename Provider, typename T>
concept IsArtifactKeyProvider = 
    std::is_base_of_v<ArtifactKeyProvider<T, Provider>, Provider> &&
    HasKeyForImpl<Provider, T>;
```

## Pre-C++20 Alternative: Static Assertions

If you're not on C++20 or prefer static assertions:

```cpp
template<typename T, typename Derived>
class ArtifactKeyProvider {
private:
    // Static check that happens at instantiation time
    static constexpr bool check_derived() {
        using DerivedType = Derived;
        static_assert(
            std::is_same_v<
                decltype(std::declval<const DerivedType&>().key_for_impl(
                    std::declval<const std::string&>(),
                    std::declval<const TilesetArtifact&>()
                )),
                ArtifactKey<T>
            >,
            "Derived class must implement key_for_impl with correct signature"
        );
        return true;
    }
    
    static constexpr bool checked = check_derived();
    
public:
    [[nodiscard]] ArtifactKey<T> key_for(const std::string& tileset_name, 
                                         const TilesetArtifact& artifact) const {
        return static_cast<const Derived*>(this)->key_for_impl(tileset_name, artifact);
    }
};
```

## Usage Examples

### Correct Implementation

```cpp
// This will compile fine
template<typename T>
class GoodProvider : public ArtifactKeyProvider<T, GoodProvider<T>> {
public:
    [[nodiscard]] ArtifactKey<T> key_for_impl(const std::string& name,
                                              const TilesetArtifact& artifact) const {
        return ArtifactKey<T>{};
    }
};
```

### Incorrect Implementation (Compile-Time Error)

```cpp
// This will fail at compile time with a clear error about not satisfying HasKeyForImpl
template<typename T>
class BadProvider : public ArtifactKeyProvider<T, BadProvider<T>> {
public:
    // Wrong return type - compile error!
    [[nodiscard]] int key_for_impl(const std::string& name,
                                   const TilesetArtifact& artifact) const {
        return 42;
    }
};
```

## Advantages of the CRTP + Concepts Approach

1. **Clear compile-time errors** when the derived class doesn't implement the interface correctly
2. **Better error messages** that explicitly state which requirement wasn't met
3. **Documentation** of the interface requirements right in the code
4. **Type safety** without runtime overhead
5. **No virtual function overhead** - everything is resolved at compile time
6. **Template parameter at class level** rather than method level, avoiding the virtual template limitation

## When to Use This Solution

This approach is ideal when:
- You can afford to have the template parameter at the class level rather than the method level
- You want compile-time polymorphism instead of runtime polymorphism
- You know the derived types at compile time
- Performance is critical (no vtable overhead)
- You want strong type checking and clear error messages

## Limitations

- No runtime polymorphism (can't store different instantiations in the same container without type erasure)
- Template parameter must be at class level, not method level
- All types must be known at compile time
- Can lead to code bloat if many instantiations are used
