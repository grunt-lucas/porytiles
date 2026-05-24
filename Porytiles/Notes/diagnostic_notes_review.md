# Diagnostic Notes Review

Guidelines for when a diagnostic should use a separate `note` vs rolling content into the main message.

## Use a separate note when

- The supplementary info references a **different source location** (e.g., "note: declared here" pointing to a
  different file/line — the clang pattern).
- The note provides a **suggested fix** that is distinct from the diagnosis itself.
- The info is optional/verbose and users may want to **filter it independently** from the main diagnostic.

## Roll into the main message when

- The info is **integral context** for the *same* finding at the *same* location.
- There is no meaningful reason a user would want to see the main message without the details (e.g., tile art
  previews, tilemap entry lists).
- The "note" is just additional lines of the same thought, not a cross-reference to another location.

## Review checklist

Diagnostic tags to audit (populate during review pass):

- [ ] *(to be populated)*
