#  Author: Marcel Simader (marcel0simader@gmail.com)
#  Date: 17.01.2024
#  (c) Marcel Simader 2024, Johannes Kepler Universität Linz

import signal
import subprocess
import sys
from io import StringIO
from threading import Event, Lock, Thread, current_thread
from time import perf_counter_ns
from typing import IO, Any, Sequence

from ferat.codes import ExitCode
from ferat.utils import (
    BAD,
    ENCODING,
    NORMAL,
    EscSeq,
    SomePath,
    fatal,
    get_show_color,
    get_show_command,
    get_timeout,
    status,
    style,
    write_encoding_agnostic,
)

#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Process Execution ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

def assert_exit_code(expected: int | set[int], actual: int) -> None:
    """
    Asserts that the given exit code is either the exact value or one of
    the values in the given list.
    """
    exps = expected if isinstance(expected, set) else {expected}
    if any(exp == actual for exp in exps):
        return
    exps_str = ", or ".join(str(exp) for exp in exps)
    if actual < 0:
        signal_name = f" ({style(BAD, signal.Signals(-actual).name)})"
    else:
        signal_name = ""
    fatal(
        ExitCode.PROCESS_FAILED,
        f"Expected exit code {exps_str!s}, but got {actual!s}{signal_name!s}",
    )

def async_pipe(
    from_io: IO[bytes],
    to_ios: Sequence[IO[str] | IO[bytes]],
    colors: Sequence[EscSeq],
    line_lock: Lock,
    interrupt: Event,
    timeout: float | None,
) -> Thread:
    timeout = -1.0 if (timeout is None) else timeout

    def write_to_streams(content: bytes | None, *, flush: bool) -> None:
        written: list[int] = []
        for to_io, color in zip(to_ios, colors):
            write_color = get_show_color() and (color is not NORMAL)
            if content is not None:
                if write_color: write_encoding_agnostic(color, to_io)
                written.append(write_encoding_agnostic(content, to_io))
            if write_color: write_encoding_agnostic(NORMAL, to_io)
            if flush: to_io.flush()
        assert (content is None) \
            or all(num_bytes == len(content) for num_bytes in written)

    def write_controller() -> None:
        buf: bytes = b""
        break_loop = False

        line_lock.acquire(True, timeout)
        while not break_loop:
            break_loop = interrupt.is_set() or \
                from_io.closed or (not from_io.readable())
            if not break_loop:
                ch = from_io.read(1)
                if len(ch) > 0: buf += ch
                else: break_loop = True
            if len(buf) >= 2:
                if buf.endswith(b"\r\n") or buf.endswith(b"\n\r"):
                    # Skip carriage returns entirely in '\n\r' or '\r\n'
                    buf = buf[:-2] + b"\n"
            elif not break_loop:
                # (not break_loop) and (not buffer_full), keep reading
                continue
            write_to_streams(buf, flush=False)
            if b"\n" in buf:
                # Give other threads opportunity to write some lines, but only
                # if we just ended a whole line (or timed out)
                if line_lock.locked(): line_lock.release()
                line_lock.acquire(True, timeout)
            buf = b""
        # Write one last NORMAL, to be sure, and then flush
        write_to_streams(None, flush=True)
        if line_lock.locked(): line_lock.release()

    t = Thread(
        name=f"ferat-worker-{id(from_io)}",
        target=write_controller,
        daemon=True
    )
    t.start()
    return t

def call_subprocess(
    args: Sequence[Any],
    expected_exit: int | set[int],
    capture_stdout: bool = True,
    capture_stderr: bool = True,
    capture_color_stdout: EscSeq = NORMAL,
    capture_color_stderr: EscSeq = NORMAL,
    shell: bool = False,
    cwd: SomePath | None = None,
) -> tuple[int, str, str, float]:
    """
    Function calling a subprocess. Potential exceptions and assertions are
    handled and logged automatically. Returns a tuple of an exit code, the
    stdout stream, the stderr stream, and the execution time in microseconds.
    """
    try:
        arg_strs = tuple(str(arg) for arg in args)
        if get_show_command():
            only_capture = False
            status(f"Invoking '{' '.join(arg_strs)}'...")
        else:
            only_capture = True
        start_time_us = perf_counter_ns() * 1e-3
        proc = subprocess.Popen(
            args=arg_strs,
            stdout=subprocess.PIPE if capture_stdout else sys.stdout,
            stderr=subprocess.PIPE if capture_stderr else sys.stderr,
            shell=shell,
            cwd=cwd,
            text=False,
            close_fds=True,
        )
        tot_time_us: float
        stdout_io, stderr_io = StringIO(), StringIO()
        timeout = None if (get_timeout() <= 0.0) else get_timeout()
        if only_capture:
            proc.wait(timeout)
            tot_time_us = (perf_counter_ns() * 1e-3) - start_time_us
            if proc.stdout is not None:
                write_encoding_agnostic(proc.stdout.read(), stdout_io)
            if proc.stderr is not None:
                write_encoding_agnostic(proc.stderr.read(), stderr_io)
        else:
            t1, t2 = None, None
            line_lock = Lock()
            interrupt = Event()
            interrupt.clear()
            try:
                if proc.stdout is not None:
                    t1 = async_pipe(
                        proc.stdout,
                        (stdout_io, sys.stdout),
                        (NORMAL, capture_color_stdout),
                        line_lock,
                        interrupt,
                        0.4,
                    )
                if proc.stderr is not None:
                    proc.stderr
                    t2 = async_pipe(
                        proc.stderr,
                        (stderr_io, sys.stderr),
                        (NORMAL, capture_color_stderr),
                        line_lock,
                        interrupt,
                        0.4,
                    )
                proc.wait(timeout)
                #  if proc.stdout is not None:
                #      proc.stdout.close()
                #  if proc.stderr is not None:
                #      proc.stderr.close()
                tot_time_us = (perf_counter_ns() * 1e-3) - start_time_us
            except Exception as exc:
                proc.kill()
                interrupt.set()
                raise exc
            finally:
                if t1 is not None: t1.join()
                if t2 is not None: t2.join()
        assert_exit_code(expected_exit, proc.returncode)
        return (
            proc.returncode,
            stdout_io.getvalue(),
            stderr_io.getvalue(),
            tot_time_us,
        )
    except subprocess.TimeoutExpired as e:
        fatal(ExitCode.PROCESS_TIMED_OUT, e)
    except (OSError, subprocess.SubprocessError) as e:
        fatal(ExitCode.PROCESS_OS_ERROR, e)
    assert False, "unhandled state"
