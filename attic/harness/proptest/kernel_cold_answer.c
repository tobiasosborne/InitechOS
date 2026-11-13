/* Factory-only cross-TU size oracle; never linked into a shipped image. */
unsigned cold_answer(unsigned value) { return value * 3u + 1u; }
