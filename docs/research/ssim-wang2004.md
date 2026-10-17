# SSIM mathematical source (initech-w06)

Local source notes acquired before implementation from Wang, Bovik, Sheikh,
and Simoncelli, *Image Quality Assessment: From Error Visibility to Structural
Similarity*, IEEE TIP 13(4), 2004, pp. 600-612,
[DOI 10.1109/TIP.2003.819861](https://www.cns.nyu.edu/pub/lcv/wang03-reprint.pdf).
These are mathematical facts transcribed into notation for this factory,
not a copy or port of the authors' MATLAB software.

- Printed p. 604, eqs. (6)-(7): luminance stabilization C1=(K1 L)^2;
  8-bit range L=255.
- Printed p. 605, eq. (13):
  SSIM=((2 mu_x mu_y+C1)(2 cov_xy+C2))/
  ((mu_x^2+mu_y^2+C1)(var_x+var_y+C2)). Negative covariance is retained.
- Printed p. 606, eqs. (14)-(16): normalized 11x11 Gaussian weights,
  sigma=1.5; weighted means, population variances and covariance.
  K1=0.01, K2=0.03. Eq. (17): arithmetic mean over the local SSIM map.
- The authors' [algorithm description](https://www.cns.nyu.edu/~lcv/ssim/ssim_index.m),
  input/output documentation, confirms valid windows only: map dimensions
  are image dimensions minus kernel dimensions plus one. No border padding.

Factory choices: convert decoded 8-bit RGB to full-range BT.601 luma
Y=0.299R+0.587G+0.114B, with no gamma transform, resizing, or downsampling.
This explicitly describes the comparison of sampled framebuffer values,
consistent with system7-decomp/specs/sys8/window-chrome.md's gamma warning.
Only fully contained windows in equal-sized, separately located crops count.
Missing/malformed inputs are errors; image similarity itself never decides
success (PRD sections 3/8, ADR-0010 CD-7, HER-11).

Acquisition limitation: shell DNS is unavailable in this lane. The primary
paper and author documentation were inspected through the web reader; the
equation and parameter notes above are the local reference. No PDF copy is
claimed, and no DOS-manual behavior is implemented by this factory tool.
