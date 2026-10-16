# LOCKED M024-M026 traces. Scene bounds retain kmain's authored geometry:
# ledger(60,80)-(360,300), chart(300,120)-(560,360), modal(140,200)-(500,280).
# Content uses CALC.EXE unchanged: flair_calc_traces.mk's local layout.
# Boot cursor is hidden until input; Saving selects the canon hourglass.
# Event source starts at (320,240). Park busy hotspot at (470,228).
FLAIR_FILM_BUSY_SPEC := m100:-12,m50:0
# Dismiss the staged Saving tableau, drag chart (+60,+40), park at (620,440).
# This is a scene demo dismissal, not a simulated successful disk write.
FLAIR_FILM_DRAG_SPEC := $(FLAIR_FILM_BUSY_SPEC),kesc,m-90:-98,l1,m60:40,l0,m100:100,m80:100,m0:70
