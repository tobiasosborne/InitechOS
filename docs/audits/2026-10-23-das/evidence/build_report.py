"""Findings were assembled and verified during the audit.
The finalized data is findings.json; render_report.py produces REPORT.md and NITS.tsv.
This wrapper does not overwrite the final human-reviewed findings.
"""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name('render_report.py')))
