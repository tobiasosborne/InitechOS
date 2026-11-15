#!/usr/bin/env python3
"""serve.py -- one interpreter for many independent-reader runs (factory).

Starting python3 costs about 0.1 s, and the host suite started some 300 of
them, which was its whole time budget. This driver runs an UNCHANGED reader
script exactly as `python3 SCRIPT ARG...` would -- runpy with __name__ ==
'__main__', its own sys.argv, its directory first on sys.path -- once per
request, capturing its standard output and exit status. It reads no .WK1
bytes and compares nothing: the readers and their callers keep every check.

Requests, one per line, tab-separated:  OUT SCRIPT ARG...
OUT is a file that receives the run's standard output (created before the
run, as a shell redirection would), or '-'.

  serve.py --serve   answers each request with "<status> <nbytes>\\n" and, for
                     OUT '-', the nbytes of standard output (else 0 bytes).
  serve.py --batch DONE
                     OUT '-' writes to our own standard output. Stops at the
                     first nonzero status and exits with it, as the former
                     `set -e` sequence of python3 calls did; after the last
                     request it writes the number of runs completed with
                     status 0 to the file DONE. The caller requires DONE to
                     hold its own request count, so a driver that ran
                     nothing can never pass.

The status follows the interpreter: sys.exit(None) 0, sys.exit(n) n & 255,
sys.exit(text) prints text and 1, an uncaught exception a traceback and 1.
ASCII-clean; deterministic.
"""
import io, os, runpy, sys, traceback


def run(out, script, args):
    """One `python3 SCRIPT ARGS > OUT` run; returns (status, inline bytes)."""
    sink = io.BytesIO() if out == '-' else open(out, 'wb')
    text = io.TextIOWrapper(sink, encoding=sys.__stdout__.encoding,
                            errors=sys.__stdout__.errors, newline=None,
                            write_through=True)
    saved_argv, saved_path, saved_stdout = sys.argv, sys.path[:], sys.stdout
    sys.argv = [script] + args
    sys.path.insert(0, os.path.dirname(os.path.abspath(script)))
    sys.stdout = text
    status = 0
    try:
        runpy.run_path(script, run_name='__main__')
    except SystemExit as e:
        if e.code is None:
            status = 0
        elif isinstance(e.code, int):
            status = e.code & 255
        else:
            sys.stderr.write('%s\n' % (e.code,))
            status = 1
    except Exception:
        traceback.print_exc()
        status = 1
    finally:
        text.flush()
        sys.stdout, sys.argv, sys.path[:] = saved_stdout, saved_argv, saved_path
        sys.stderr.flush()
    data = sink.getvalue() if out == '-' else b''
    text.detach()
    if out != '-':
        sink.close()
    return status, data


def requests(stream):
    for line in stream:
        fields = line.rstrip('\n').split('\t')
        if len(fields) < 2 or not fields[0] or not fields[1]:
            sys.stderr.write('serve.py: malformed request %r\n' % line)
            sys.exit(2)
        yield fields[0], fields[1], fields[2:]


def main():
    mode = sys.argv[1] if len(sys.argv) in (2, 3) else ''
    if mode == '--serve' and len(sys.argv) == 2:
        reply = sys.stdout.buffer
        for out, script, args in requests(sys.stdin):
            status, data = run(out, script, args)
            reply.write(b'%d %d\n' % (status, len(data)))
            reply.write(data)
            reply.flush()
        return 0
    if mode == '--batch' and len(sys.argv) == 3:
        done = 0
        for out, script, args in requests(sys.stdin):
            status, data = run(out, script, args)
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
            if status:
                return status
            done += 1
        with open(sys.argv[2], 'w') as f:
            f.write('%d\n' % done)
        return 0
    sys.stderr.write(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main())
