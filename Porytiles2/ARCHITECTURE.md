# Architecture
This document describes the high-level architecture of Porytiles.
If you want to familiarize yourself with the code base, you are just in the right place!

## Bird's Eye View
TODO: summarize the architecture

TODO: explain how the codemap layer dependencies work:
- utilities can depend on nothing
- xcut can depend on utilities ONLT
- domain can depend on utilities and xcut
- app can depend on all of the above
- infra can depend on all of the above, including app

## Codemap
This section talks briefly about various important directories and data structures.
Pay attention to the Architecture Invariant sections.
They often talk about things which are deliberately absent in the source code.

### `app/config`

### `app/use_cases`

### `di`

### `domain/algorithms`

### `domain/config`

### `domain/models`

### `domain/repos`

### `domain/services`

### `infra/config`

### `infra/repos`

### `infra/services`

### `utilities`

### `xcut`

## Cross-Cutting Concerns
TODO

## Domain
TODO: summarize the core domain types

## Links

### Inspiration Documents
https://matklad.github.io//2021/02/06/ARCHITECTURE.md.html
https://github.com/rust-lang/rust-analyzer/blob/d7c99931d05e3723d878bea5dc26766791fa4e69/docs/dev/architecture.md
