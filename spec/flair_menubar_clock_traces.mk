# LOCKED tdnl.13 R3.6: real mouse packets wake the existing cooperative pump;
# no synthetic time-setting action enters the guest. The harness pins the RTC
# at00:55:55; the gate-only1200-PIT-tick lifetime spans the real minute boundary.
# Initial, per-event and final frames make a short Rule-14 clock-tick clip.
FLAIR_MENUBAR_CLOCK_SPEC := m1:0,m1:0
