# Best Use Study

## Source Prototype

The source project demonstrates a useful idea: build a character set from a
small QA dataset, encode each question and answer as a padded one-hot vector,
serialize the result, and answer new questions by nearest-neighbor search.

The prototype also shows the risks that an open-source reproduction should
address first:

- the encoder and inference reader disagree on the serialized field names
- inference hard-codes dimensions that already exist in metadata
- fixed buffers can be overrun by larger embedding files
- generated models are hard to inspect and hard to test

## Best Open-Source Use

The best use is an educational local retrieval engine, not a general chatbot.
The project should make every step visible:

- dataset parsing
- character-set construction
- one-hot vector generation
- model serialization
- query encoding
- similarity scoring

That focus gives the project a clear audience: students, systems programmers,
and developers who want a small baseline before introducing larger NLP
dependencies.

## Suggested Direction

The recommended direction is a stable C99 CLI and small reusable library.

Rejected directions:

- General AI assistant: the method is too shallow for open-domain language use.
- Large vector database: this code is better as a readable baseline than as a
  storage engine.
- Python-first rewrite: it would make experimentation easier, but it would lose
  the original project's value as a low-level C learning artifact.

## Final Decision

Build `charvec-qa`: a dependency-free C99 command-line tool that can build,
inspect, and query a small QA vector model. Keep the model format text-based and
documented. Store both source text and vectors so retrieval is transparent and
answers do not depend on fragile reverse-decoding.

## Naming Convention

- Repository: `charvec-qa`
- Binary: `charvec`
- Model files: `*.cvec`
- Public symbols: `charvec_*`
- Header guard: `CHARVEC_H`
- Source files: lowercase with underscores if needed

## Repository Structure Produced

```text
charvec-qa/
|-- Makefile
|-- README.md
|-- LICENSE
|-- include/charvec.h
|-- src/charvec.c
|-- src/main.c
|-- examples/dataset.txt
|-- docs/FORMAT.md
|-- docs/USE_STUDY.md
`-- tests/smoke.sh
```

