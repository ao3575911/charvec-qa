# CVEC Model Format

`charvec-qa` uses a plain-text model format named `CVEC1`.

## Header

```text
CVEC1
pairs <count>
max_len <longest question or answer length>
charset_size <number of unique bytes>
charset_hex <space-separated uppercase byte values>
---
```

The charset is stored as hexadecimal byte values to keep spaces, punctuation,
and control characters unambiguous.

## Records

Each record contains original text as hex plus its encoded vectors:

```text
q_text_hex <hex bytes>
a_text_hex <hex bytes>
q_bits <flattened one-hot bits>
a_bits <flattened one-hot bits>
---
```

The bit length for every vector must equal:

```text
max_len * charset_size
```

Bits are left-aligned with zero padding at the end. Query vectors use the same
charset and dimensions loaded from the model header.

