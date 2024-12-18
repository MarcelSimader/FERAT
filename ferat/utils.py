#  Author: Marcel Simader (marcel0simader@gmail.com)
#  Date: 08.11.2023
#  (c) Marcel Simader 2023, Johannes Kepler Universität Linz

import gzip
import inspect
import sys
from io import BytesIO, RawIOBase, StringIO, TextIOBase
from os import PathLike
from pathlib import Path
from typing import (
    IO,
    Any,
    Callable,
    Final,
    NewType,
    NoReturn,
    ParamSpec,
    TypeAlias,
    TypedDict,
    TypeVar,
)

#  Globals {{{
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Globals ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SomePath: TypeAlias = str | PathLike | Path

_R = TypeVar("_R")
_P = ParamSpec("_P")
EscSeq = NewType("EscSeq", str)

Encoding = TypedDict("Encoding", {"encoding": str, "errors": str})
ENCODING: Encoding
try:
    ENCODING = {"encoding": sys.getdefaultencoding(), "errors": "strict"}
except Exception:
    ENCODING = {"encoding": "utf-8", "errors": "strict"}

_INDENT_BASE: Final[int] = 0
_INDENT_CHANGE: Final[int] = 4
_indent: int = _INDENT_BASE

_TIMEOUT: float = 0
_SHOW_COLOR: bool = True
_SHOW_COMMAND: bool = True
_SHOW_STATUS: bool = True
_PROFILE: bool = False

def set_timeout(v: float) -> None:
    global _TIMEOUT
    _TIMEOUT = v

def set_show_color(v: bool) -> None:
    global _SHOW_COLOR
    _SHOW_COLOR = v

def set_show_status(v: bool) -> None:
    global _SHOW_STATUS
    _SHOW_STATUS = v

def set_show_command(v: bool) -> None:
    global _SHOW_COMMAND
    _SHOW_COMMAND = v

def set_profile(v: bool) -> None:
    global _PROFILE
    _PROFILE = v

def get_timeout() -> float:
    return _TIMEOUT

def get_show_color() -> bool:
    return _SHOW_COLOR

def get_show_status() -> bool:
    return _SHOW_STATUS

def get_show_command() -> bool:
    return _SHOW_COMMAND

def get_profile() -> bool:
    return _PROFILE

#  }}}

#  IO {{{
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ IO ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

def write_encoding_agnostic(
    data: bytes | bytearray | str, stream: IO[bytes] | IO[str]
) -> int:
    is_str = isinstance(data, str)
    if isinstance(stream, (RawIOBase, BytesIO)):
        if isinstance(data, str):
            return stream.write(data.encode(**ENCODING))
        else:
            return stream.write(data)
    elif isinstance(stream, (TextIOBase, StringIO)):
        if isinstance(data, str): return stream.write(data)
        else: return stream.write(data.decode(**ENCODING))
    else:
        raise TypeError(f"Unsupported stream type {type(stream)!r}")

def is_gzipped(file_path: SomePath) -> bool:
    try:
        with open(file_path, 'rb') as file:
            magic_number = file.read(2)
        return magic_number == b'\x1f\x8b'
    except OSError as oerr:
        warn(f"Encountered error while checking for gzipped file: {oerr!s}")
        return False

def open_zip_agnostic(file_path: SomePath, mode: str) -> IO[str]:
    # TODO(Marcel): I feel like there is a nicer way to force files to open in
    #               text mode while also supporting gzip...
    # NOTE: We force '...t' text mode here
    out: gzip.GzipFile | IO[Any]
    if is_gzipped(file_path):
        out = gzip.open(file_path, f"{mode.replace('t', '')!s}t", **ENCODING)
    else:
        out = open(file_path, f"{mode.replace('t', '')!s}t", **ENCODING)
    assert isinstance(out, TextIOBase)
    return out

#  }}}

#  Color Printing {{{
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Color Printing ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# escape sequence constants
ESC: Final[EscSeq] = EscSeq("\x1B")
CSI: Final[EscSeq] = EscSeq(f"{ESC}[")
GRAPHIC: Final[Callable[[int], EscSeq]] = lambda d: EscSeq(f"{CSI}{d}m")

# text and color modifies
RESET_ALL: Final[EscSeq] = GRAPHIC(0)
NORMAL: Final[EscSeq] = RESET_ALL
DIM: Final[EscSeq] = GRAPHIC(2)
# foreground
RED: Final[EscSeq] = GRAPHIC(31)
GREEN: Final[EscSeq] = GRAPHIC(32)
YELLOW: Final[EscSeq] = GRAPHIC(33)
BRIGHT_RED: Final[EscSeq] = GRAPHIC(91)
BRIGHT_GREEN: Final[EscSeq] = GRAPHIC(92)
BRIGHT_BLUE: Final[EscSeq] = GRAPHIC(94)
BRIGHT_MAGENTA: Final[EscSeq] = GRAPHIC(95)
BRIGHT_CYAN: Final[EscSeq] = GRAPHIC(96)
BRIGHT_BLACK: Final[EscSeq] = GRAPHIC(90)
# background
BACK_RED: Final[EscSeq] = GRAPHIC(41)
BACK_GREEN: Final[EscSeq] = GRAPHIC(42)
# definitions
FERAT: Final[EscSeq] = BRIGHT_MAGENTA
GOOD: Final[EscSeq] = GREEN
BAD: Final[EscSeq] = BRIGHT_RED
WORSE: Final[EscSeq] = RED
VERY_BAD: Final[EscSeq] = EscSeq(NORMAL + BACK_RED)
IMPORTANT: Final[EscSeq] = BRIGHT_BLUE
WARNING: Final[EscSeq] = YELLOW
OTHER_STDOUT: Final[EscSeq] = BRIGHT_BLACK
OTHER_STDERR: Final[EscSeq] = EscSeq(DIM + RED)
DEBUG: Final[EscSeq] = BRIGHT_CYAN

def style(s: EscSeq, o: Any, /, *, clear: bool = True) -> str:
    if not get_show_color(): return f"{o!s}"
    return f"{s}{o!s}{RESET_ALL if clear else ''}"

def print_style(
    s: EscSeq,
    o: Any,
    /,
    *,
    clear: bool = True,
    end: str = "\n",
    file: IO[str] = sys.stdout,
    flush: bool = True,
) -> None:
    print(style(s, o, clear=clear), end=end, file=file, flush=flush)

#  }}}

#  Logging Functions {{{
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Logging Functions ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

def status(
    o: Any,
    /,
    color: EscSeq = FERAT,
    end: str = "\n",
    file: IO[str] = sys.stdout,
    flush: bool = True,
) -> None:
    indent = "" if (_indent <= _INDENT_BASE) else (" " * _indent)
    o = str(o)
    for line in (o.splitlines() if (len(o) > 0) else (o,)):
        print_style(color, f"[FERAT]{indent}", end="", file=file)
        print(f" {line}", end=end, file=file, flush=flush)

def warn(o: Any, /, color: EscSeq = WARNING, end: str = "\n") -> None:
    status(style(color, o), color, end, file=sys.stderr, flush=True)

def debug(o: Any, /, color: EscSeq = DEBUG, end: str = "\n") -> None:
    status(style(color, o), color, end, file=sys.stdout, flush=True)

class FERATFatalError(Exception):

    exit_code: int
    o: Any
    color: EscSeq
    end: str

    def __init__(
        self,
        exit_code: int,
        o: Any,
        /,
        color: EscSeq,
        end: str = "\n",
    ):
        super(FERATFatalError, self).__init__()
        self.exit_code = exit_code
        self.o = o
        self.color = color
        self.end = end

    def exit(self) -> NoReturn:
        status(
            f"FATAL: {self.o!s} \n(exiting with code {self.exit_code})",
            self.color,
            self.end,
            file=sys.stderr,
            flush=True,
        )
        sys.exit(self.exit_code)

def fatal(
    exit: int,
    o: Any,
    /,
    color: EscSeq = VERY_BAD,
    end: str = "\n",
) -> NoReturn:
    global _indent
    _indent = _INDENT_BASE
    if isinstance(o, BaseException):
        o = f"Exception '{o!s}' of type {type(o).__name__!r}"
    raise FERATFatalError(exit, o, color, end)

def status_function(func: Callable[_P, _R]) -> Callable[_P, _R]:
    """
    Wraps every function with this decorator in two status messages. If a doc
    string is specified on the function, it will be printed as well.
    """

    def __wrapped__(*args: _P.args, **kwargs: _P.kwargs) -> _R:
        global _indent
        if _SHOW_STATUS:
            status(f"Calling {style(IMPORTANT, func.__name__)}...")
            _indent += _INDENT_CHANGE
            if func.__doc__ is not None:
                for docstr_line in inspect.cleandoc(func.__doc__).splitlines():
                    status(style(DIM, f">>> {docstr_line}"))
            _indent -= _INDENT_CHANGE
        ret = func(*args, **kwargs)
        return ret

    __wrapped__.__name__ = f"wrapped_{func.__name__!s}"
    __wrapped__.__doc__ = func.__doc__
    return __wrapped__

#  }}}
# vim: foldmethod=marker
