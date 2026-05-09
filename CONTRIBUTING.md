# Contributing

`charvec-qa` is intentionally small. Contributions should keep the project easy
to read, build, and test on a plain C toolchain.

## Local Checks

```sh
make test
```

For memory-sensitive changes, also run a sanitizer build:

```sh
mkdir -p build
cc -std=c99 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -g -Iinclude src/main.c src/charvec.c -o build/charvec_asan
./build/charvec_asan build -i examples/dataset.txt -o build/asan.cvec
./build/charvec_asan ask -m build/asan.cvec -q "Who created C?"
```

## Guidelines

- Keep the model format documented in `docs/FORMAT.md`.
- Keep parsing bounded and explicit.
- Avoid new dependencies unless they are clearly optional.
- Add or update smoke coverage for user-visible behavior changes.

