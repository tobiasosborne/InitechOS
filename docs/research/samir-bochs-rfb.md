# SAMIR Bochs keyboard transport (initech-hdlb)

Source: [RFC 6143, section 7.5.4](https://www.rfc-editor.org/rfc/rfc6143.html#section-7.5.4).
Read alongside the existing section 7.5.5 pointer transport in
`harness/emu/bochs.c` and ADR-0009 DEC-08's scripted USE/LIST oracle.

The client KeyEvent packet is eight bytes: type 4, one-byte down flag,
two zero padding bytes, then a big-endian 32-bit X keysym. A release uses
down flag zero. Ordinary ASCII keys use their character value; Return is
0xff0d. The SAMIR trace needs only lowercase letters, space, period and
Return. Both edges go through Bochs's existing RFB connection and hence
its emulated keyboard, after the complete SHELL-READY serial line.

The screenshot must wait until every key release has been sent. It then
uses WL-0170's existing stable-frame and lossless-snapshot capture, with
the original QEMU SAMIR still as the independent pixel oracle.
