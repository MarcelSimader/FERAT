#  Author: Marcel Simader (marcel.simader@jku.at)
#  Date: 14.11.2023
#  (c) Marcel Simader 2023, Johannes Kepler Universität Linz

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping, MutableMapping, final

from ferat.utils import BAD, GOOD, SomePath, status, style, warn

#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

def _dep_status(pad: int, dep: Path, okay: bool, msg: str = "") -> None:
    if okay: stat = GOOD, 'FOUND'
    else: stat = BAD, 'NOT FOUND'
    status(f"{dep.name:.<{pad + 2}}: {style(*stat)} ({msg})")

#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Classes ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class Dependencies:

    _base_dir: Path
    _deps: MutableMapping[Path, bool]

    def __init__(self, dir_: SomePath, *initials: SomePath):
        self._base_dir = Path(dir_)
        self._deps = dict()
        for initial in initials:
            self.add(initial)

    @property
    def base_dir(self) -> Path:
        return self._base_dir

    @property
    def deps(self) -> Mapping[Path, bool]:
        return {**self._deps}

    def add(self, *deps: SomePath) -> None:
        for dep in deps:
            if dep in self._deps:
                raise ValueError(f"Dependency {dep!r} already added")
            self._deps[Path(self._base_dir).joinpath(dep)] = False

    def check(self) -> bool:
        longest_pad = max(len(p.name) for p in self._deps)
        for dep in self._deps:
            self._deps[dep] = False
            if not dep.exists():
                _dep_status(longest_pad, dep, False, "does not exist")
                continue
            if not dep.is_file():
                _dep_status(longest_pad, dep, False, "not a file")
                continue
            try:
                if sys.platform != "win32":
                    # RW[X]-RW[X]-RW[X]
                    if (dep.stat().st_mode & 0o111) == 0:
                        _dep_status(longest_pad, dep, False, "not executable")
                        continue
                _dep_status(longest_pad, dep, True, "OK")
                self._deps[dep] = True
            except OSError as oserr:
                warn(oserr)
                # We can just assume it exists? Probably some OS
                # compatibility error stuff
                self._deps[dep] = True
        return self.all_found()

    def all_found(self) -> bool:
        return all(self._deps.values())

    def __bool__(self) -> bool:
        return self.all_found()

    def __truediv__(self, s: SomePath) -> Path:
        return self._base_dir.joinpath(s.name if isinstance(s, Path) else s)
