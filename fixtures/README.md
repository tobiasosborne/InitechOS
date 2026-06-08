# fixtures/ — the frame still(s) and golden files

The reference frame is a *fixture*, not inspiration (PRD §3). This holds the
*Office Space* "Saving tables to disk..." still(s) and the golden files the
oracles diff against: FAT images, `.dbf`/`.mdx` round-trip goldens, the
compiler corpus outputs, and the SSIM-guide frame crops.

Golden discipline (CLAUDE.md Rule 6): a golden that never caught a
regression is decoration. After capturing one, perturb the implementation
by one branch/constant, confirm the oracle goes red, then restore. The bar
is "the golden has caught a real regression," not "the golden was committed
first."

Governed by: **PRD §3, §8, Appendix A.**
