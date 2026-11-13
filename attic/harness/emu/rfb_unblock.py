#!/usr/bin/env python3
"""rfb_unblock.py -- headless-Bochs DIAGNOSIS helper (beads initech-6pj/564).

Bochs in this environment is built with ONLY the RFB (VNC) display plugin; it
blocks ~30s waiting for a VNC client and never self-proceeds to run the guest,
so there is no clean headless path out of the box (no nogui/x/sdl). This minimal
RFB 3.3 client connects to the Bochs RFB server, completes the handshake
(security type None, no auth), and HOLDS the connection so Bochs stops waiting
and runs the guest. Serial is captured via `com1: mode=file` in the bochsrc.

THIS IS DEV/DIAGNOSIS TOOLING, NOT THE SHIPPED HARNESS. The production Bochs
driver (harness/emu/bochs.c, to be written) must implement this handshake in C
(the harness is C-only, Law 3) OR use the `bochs-term` display plugin (now
installed) which may avoid the RFB wait entirely -- the next agent should try
`display_library: term` first as it is simpler than driving RFB.

Usage:  rfb_unblock.py [port] [hold_seconds]   (defaults: 5900, 12)

Proven recipe (SeaBIOS path; reproduces the VBE-E00 failure -- see WL-0014):
    cp build/tracer_boot.img /tmp/bx.img; rm -f /tmp/bx.img.lock /tmp/bx.serial
    printf 'c\n' > /tmp/bx.rc
    cat > /tmp/bx.cfg <<EOF
    romimage: file=/usr/share/seabios/bios.bin
    vgaromimage: file=/usr/share/vgabios/vgabios.bin
    megs: 32
    vga: extension=vbe
    cpu: model=pentium, ips=50000000
    ata0: enabled=1, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14
    ata0-master: type=disk, path="/tmp/bx.img", mode=flat, cylinders=2, heads=2, spt=32
    boot: disk
    com1: enabled=1, mode=file, dev=/tmp/bx.serial
    display_library: rfb
    clock: sync=none
    EOF
    timeout 22 bochs -q -f /tmp/bx.cfg -rc /tmp/bx.rc >/tmp/bx.out 2>&1 &
    sleep 3; python3 harness/emu/rfb_unblock.py 5900 15; wait
    tr -d '\r' < /tmp/bx.serial   # -> S1 JMP2 S2 VBE-E00 ERR-VBE
"""
import socket, sys, time

port = int(sys.argv[1]) if len(sys.argv) > 1 else 5900
hold = float(sys.argv[2]) if len(sys.argv) > 2 else 12.0

deadline = time.time() + 25.0
s = None
while time.time() < deadline:
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=2)
        break
    except OSError:
        time.sleep(0.25)
if s is None:
    print("RFB-UNBLOCK: could not connect to port %d" % port, file=sys.stderr)
    sys.exit(1)

s.settimeout(6)
try:
    ver = s.recv(12)                      # server ProtocolVersion "RFB 003.00x\n"
    print("RFB-UNBLOCK: server=%r" % ver, file=sys.stderr)
    s.sendall(b"RFB 003.003\n")           # negotiate the simplest variant
    sec = s.recv(4)                       # 3.3: 4-byte security type (1 == None)
    print("RFB-UNBLOCK: sectype=%r" % sec, file=sys.stderr)
    s.sendall(b"\x01")                    # ClientInit: shared-flag = 1
    try:
        init = s.recv(4096)               # ServerInit (dims + pixfmt + name)
        print("RFB-UNBLOCK: serverinit %d bytes" % len(init), file=sys.stderr)
    except socket.timeout:
        print("RFB-UNBLOCK: no ServerInit (ok -- guest may already be running)",
              file=sys.stderr)
except Exception as e:
    print("RFB-UNBLOCK: handshake note: %r" % e, file=sys.stderr)

time.sleep(hold)                          # hold the connection so Bochs keeps running
print("RFB-UNBLOCK: done", file=sys.stderr)
