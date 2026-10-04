#!/usr/bin/env python3
"""Fresh-copy control and long-path reproduction of H01. One QEMU only."""
from audit_driver import Driver

d = Driver()
try:
    d.start('menu-repro', 'images/pristine-boot.img', 'images/pristine-data.img')
    d.pause(0.5)
    d.click(600, 64, 2)
    d.note('Short cross-title control: File -> View -> by Icons')
    d.move(42, 28)
    d.button(True)
    d.rel(77, 0)
    d.pause(0.3)
    d.rel(16, 22)
    d.pause(0.3)
    d.shot('78-short-cross-menu-held')
    d.button(False)
    d.pause(1)
    d.shot('79-short-cross-menu-result')
    d.note('H01: 80 distinct jitter moves before switching title')
    d.move(42, 28)
    d.button(True)
    d.rel(18, 22)
    d.pause(0.3)
    for _ in range(40):
        d.rel(4, 0)
        d.pause(0.08)
        d.rel(-4, 0)
        d.pause(0.08)
    d.rel(59, -22)
    d.pause(0.4)
    d.rel(16, 22)
    d.pause(0.5)
    d.shot('80-long-cross-menu-held')
    d.button(False)
    d.pause(1)
    d.shot('81-long-cross-menu-wrong-command')
    d.tail(22)
finally:
    if d.proc is not None:
        d.quit()
