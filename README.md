# charvec-qa

`charvec-qa` is a small, dependency-free C99 project for transparent local
question-answer retrieval using character-position one-hot vectors.

It is inspired by the original `Vector-Based-NLP-Engine-C` prototype, but the
encoder, model format, and inference path are deliberately kept in one
compatible system. The goal is not to compete with modern embedding models. The
goal is to make vectorization, serialization, and nearest-neighbor search easy
to inspect, test, and teach.

## Best Use

Use this project as an educational baseline for:

- showing how text becomes a fixed-width vector without external libraries
- building a tiny local FAQ retriever for small curated datasets
- testing serialization contracts between training and inference
- comparing simple symbolic vectors against larger NLP systems

## Quickstart

Copy-paste commands are collected in [CHEATSHEET.md](CHEATSHEET.md).

```sh
git clone https://github.com/ao3575911/charvec-qa.git
cd charvec-qa
make test
./build/charvec build -i examples/dataset.txt -o build/sample.cvec
./build/charvec ask -m build/sample.cvec -q "Who created C?"
```

Expected answer:

```text
answer: Dennis Ritchie created C at Bell Labs.
```

## Build

```sh
make
```

## Run

Build a model from the sample dataset:

```sh
./build/charvec build -i examples/dataset.txt -o build/sample.cvec
```

Ask a question:

```sh
./build/charvec ask -m build/sample.cvec -q "Who created C?"
```

Inspect model metadata:

```sh
./build/charvec inspect -m build/sample.cvec
```

Run the smoke tests:

```sh
make test
```

## Dataset Format

Datasets are plain text records:

```text
Q: What is C?
A: C is a general-purpose programming language.
---
Q: Who created C?
A: Dennis Ritchie created C at Bell Labs.
---
```

Blank lines are ignored. Records must contain one `Q:` line and one `A:` line.

## Repository Layout

```text
.
|-- Makefile
|-- README.md
|-- LICENSE
|-- include/
|   `-- charvec.h
|-- src/
|   |-- charvec.c
|   `-- main.c
|-- examples/
|   `-- dataset.txt
|-- docs/
|   |-- FORMAT.md
|   `-- USE_STUDY.md
`-- tests/
    `-- smoke.sh
```

## Publication Status

This repository is ready for small-scale educational use. It includes a
documented model format, example data, smoke tests, CI, MIT license, and a
copy-paste command cheatsheet.
