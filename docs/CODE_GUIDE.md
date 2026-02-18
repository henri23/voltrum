# Code Guide

This file captures general coding conventions.

## 1. String Types

1. Prefer `String` and `Const_String<N>` over raw `char*`/`const char*`.
2. Keep raw C strings only where required by third-party APIs.
3. For persistent inline storage, prefer `Const_String<N>` and explicit conversions.

## 2. Internal Linkage Style

1. Prefer `INTERNAL_FUNC` for file-local helper functions.
2. Prefer `internal_var` for file-local static data where appropriate.
3. Avoid anonymous namespaces for internal helpers.

## 3. State Ownership

1. Do not hide mutable state in internal static/singleton component state.
2. Allocate mutable state at higher-level ownership boundaries and pass pointers/references explicitly.
3. Keep APIs explicit about state ownership via parameters.

## 4. Naming and Readability

1. Use descriptive names for variables and functions.
2. Keep temporary abbreviations only for common short forms (`id`, `dt`, etc.).
3. Prefer straightforward control flow and small helper functions over deeply nested logic.

## 5. UI Animation Pattern

1. Reuse shared animation helpers instead of duplicating per-widget math.
2. Prefer exponential-decay tracking for hover/reveal transitions.
3. Use stable IDs/keys for persistent per-widget animation state.
