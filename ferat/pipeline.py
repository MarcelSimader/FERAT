#! /usr/bin/env python3
#  Author: Marcel Simader (marcel.simader@jku.at)
#  Date: 08.11.2023
#  (c) Marcel Simader 2023, Johannes Kepler Universität Linz

import os
import re
import shutil
import sys
from argparse import (
    ArgumentError,
    ArgumentParser,
    ArgumentTypeError,
    BooleanOptionalAction,
    Namespace,
)
from functools import lru_cache, reduce
from operator import or_
from pathlib import Path
from time import perf_counter_ns
from typing import Final, Iterable, Iterator, NoReturn, Sequence, final

from ferat.codes import ExitCode
from ferat.deps import Dependencies
from ferat.proc import call_subprocess
from ferat.utils import (
    BAD,
    ENCODING,
    FERAT,
    GOOD,
    IMPORTANT,
    OTHER_STDERR,
    OTHER_STDOUT,
    EscSeq,
    FERATFatalError,
    debug,
    fatal,
    get_profile,
    get_show_color,
    open_zip_agnostic,
    set_profile,
    set_show_color,
    set_show_command,
    set_show_status,
    set_timeout,
    status,
    status_function,
    style,
    warn,
)

#  Globals {{{
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Globals ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# TODO: Always keep this up-to-date... please.
VERSION: Final[str] = "v0.9.0"

@final
class DepNames:
    ijtihad: Final[str] = "ijtihad"
    kissat: Final[str] = "kissat"
    cadical: Final[str] = "cadical"
    drat_trim: Final[str] = "drat-trim"
    lrat_trim: Final[str] = "lrat-trim"
    ferat_tools: Final[str] = "ferat-tools"

    @classmethod
    def values(cls) -> Iterator[str]:
        return (
            getattr(cls, k)
            for k, v in cls.__annotations__.items()
            if isinstance(getattr(cls, k), str) and v is Final[str]
        )

LRAT_DEPENDENCIES: Final[set[str]] = {DepNames.cadical, DepNames.lrat_trim}

@final
class Commands:
    VERSION: Final[str] = "version"
    GENERATE: Final[str] = "generate"
    CHECK: Final[str] = "check"

@final
class ProfileNames:
    QBF_SOLVE: Final[str] = "qbf_solve"
    GEN_RAT_PROOF: Final[str] = "gen_rat_proof"
    CHECK_RAT_PROOF: Final[str] = "check_rat_proof"
    CHECK_EXPANSION: Final[str] = "check_expansion"
    GEN_FERAT_PROOF: Final[str] = "gen_ferat_proof"
    SPLIT_FERAT: Final[str] = "split_ferat"
    TOTAL: Final[str] = "total"

def start_profile() -> int:
    return int(1e-3 * perf_counter_ns())

def end_profile(
    profile_name: str, time_us: int, *, no_start: bool = False
) -> None:
    if get_profile():
        if not no_start: time_us = int((1e-3 * perf_counter_ns()) - time_us)
        debug(f"{profile_name} took {time_us} µs")

@lru_cache
def RE_DIMACS_LINE(
    *xs: str, flags: Iterable[re.RegexFlag] = (re.IGNORECASE, re.MULTILINE)
) -> re.Pattern:
    """
    Creates a regular expression to match any line of form:
        DIMACS_LINE(x_i) := { " " } x0 { " " { " " } x_i }_i .
    This is for simplifying the general syntax of information embedded in
    DIMACS headers.
    """
    _SPACES = r"\s+" # We need this for older Python versions
    return re.compile(f"^\\s*{_SPACES.join(xs)}", reduce(or_, flags))

#  }}}

#  Pipeline Functions {{{
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Pipeline Functions ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#  ~~~~~~~~~~~~~~~~~~~~ Generate AND Check ~~~~~~~~~~~~~~~~~~~~

#  Dependencies {{{
#  ~~~~~~~~~~~~~~~~~~~~ Dependencies ~~~~~~~~~~~~~~~~~~~~
@status_function
def init_dependencies(dependencies_dir: Path, lrat: bool) -> Dependencies:
    status(f"Checking in '{dependencies_dir.absolute()}'")
    dependencies = Dependencies(dependencies_dir)
    for name in DepNames.values():
        if (not lrat) and (name in LRAT_DEPENDENCIES): continue
        dependencies.add(name)
    if not dependencies.check():
        fatal(ExitCode.DEPS_NOT_FOUND, "Missing dependencies")
    return dependencies

#  }}}

#  check_rat_proof {{{
#  ~~~~~~~~~~~~~~~~~~~~ check_rat_proof ~~~~~~~~~~~~~~~~~~~~
@status_function
def check_rat_proof(
    dependencies: Dependencies,
    cnf: Path,
    rat: Path,
    lrat: bool,
    simple_cnf: Path | None = None,
    simple_rat: Path | None = None,
) -> tuple[Path, Path]:
    """
    Checking the correctness of the RAT proof, while optimizing it and
    trimming the input expansion. (Checker drat-trim by Wetzler,
    Heule, and Hunt, 2014.)
    """
    # TODO: Update doc-string for LRAT
    start_time_us = start_profile()
    try:
        _, stdout, _, _ = call_subprocess(
            args=(
                dependencies / DepNames.lrat_trim,
                "--no-binary",
                cnf,
                rat,
                *(() if (simple_rat is None) else (simple_rat,)),
                *(() if (simple_cnf is None) else (simple_cnf,)),
            ) if lrat else (
                dependencies / DepNames.drat_trim,
                cnf,
                rat,
                "-I", # force ASCII parse mode
                *(() if (simple_cnf is None) else ("-c", simple_cnf)),
                *(() if (simple_rat is None) else ("-l", simple_rat)),
            ),
            capture_stdout=True,
            capture_stderr=True,
            capture_color_stdout=OTHER_STDOUT,
            capture_color_stderr=OTHER_STDERR,
            expected_exit={10, 20}
            if lrat else {0, 1}, # We have to include 1 here as well, even
            # though 0 is the "success" condition, because drat-trim will
            # output an "error" when a proof is trivially unsat. For us, it
            # doesn't matter if it is trivial or not.
        )
        # NOTE: See comment for "expected_exit" argument. We still see
        #       's VERIFIED' when it is trivially unsat!
        if RE_DIMACS_LINE("s", r"VERIFIED\s*$").search(stdout) is None:
            fatal(
                ExitCode.INVALID_RAT_PROOF, style(BAD, "RAT proof is invalid")
            )
        status(style(GOOD, "RAT proof is valid"))
        return_cnf, return_rat = cnf, rat
        output_warning = False
        if simple_cnf is not None:
            # If the output of "drat-trim" is trivially unsatisfiable,
            # there will not be an optimized RAT or trimmed expansion.
            if simple_cnf.is_file():
                # If we want to extract the trimmed CNF, we need to copy over
                # the comments of the CNF to preserve the mappings
                with (
                    open_zip_agnostic(cnf, "r") as cnf_file,
                    open_zip_agnostic(simple_cnf, "r+") as simple_cnf_file,
                ):
                    # We need this list, because comments must go first in the
                    # final output
                    # WARNING: The 'c o' mapping comment is ALREADY EXPORTED by
                    #          our modified version of drat-trim, when a core
                    #          is extracted using '-c ...'
                    lines = [
                        *(
                            f"{match_[0]}\n"
                            for match_ in RE_DIMACS_LINE("c", "x", ".*$")
                            .finditer(cnf_file.read())
                        ),
                        *simple_cnf_file.readlines(),
                    ]
                    simple_cnf_file.seek(0, os.SEEK_SET)
                    simple_cnf_file.truncate(0)
                    simple_cnf_file.writelines(lines)
                return_cnf = simple_cnf
            else:
                output_warning = True
        if simple_rat is not None:
            # Again, if trivially unsat, we don't get a simplified RAT proof.
            if simple_rat.is_file(): return_rat = simple_rat
            else: output_warning = True
        if output_warning:
            warn(
                "Trivially unsatisfiable formula or RAT proof, no"
                " RAT or CNF simplification by drat-trim"
            )
        return return_cnf, return_rat
    finally:
        end_profile(ProfileNames.CHECK_RAT_PROOF, start_time_us)

#  }}}

#  check_expansion {{{
#  ~~~~~~~~~~~~~~~~~~~~ check_expansion ~~~~~~~~~~~~~~~~~~~~
@status_function
def check_expansion(
    dependencies: Dependencies,
    qbf: Path,
    cnf: Path,
) -> None:
    """
    Checking the correctness of the FERAT proof's expansion step using the
    original QBF and the expanded CNF clauses. (FERAT-tools by Simader, and
    Seidl in 2024)
    """
    returncode, _, _, time_us = call_subprocess(
        args=(
            dependencies / DepNames.ferat_tools,
            qbf,
            cnf,
        ),
        capture_stdout=True,
        capture_stderr=True,
        capture_color_stdout=OTHER_STDOUT,
        capture_color_stderr=OTHER_STDERR,
        expected_exit={10, 20},
    )
    try:
        if returncode != 10:
            fatal(
                ExitCode.INVALID_FERAT_PROOF,
                f"{style(BAD, 'FERAT proof is invalid')}"
                f" (expansion is unsound)",
            )
        status(f"{style(GOOD, 'FERAT proof is valid')} (expansion is sound)")
    finally:
        end_profile(ProfileNames.CHECK_EXPANSION, int(time_us), no_start=True)

#  }}}

#  ~~~~~~~~~~~~~~~~~~~~ Generate ONLY ~~~~~~~~~~~~~~~~~~~~

#  solve_qbf {{{
#  ~~~~~~~~~~~~~~~~~~~~ solve_qbf ~~~~~~~~~~~~~~~~~~~~
@status_function
def solve_qbf(
    dependencies: Dependencies,
    input_: Path,
    cnf: Path,
) -> None:
    """
    Calling the QBF solver. This step extracts the expanded clauses of the
    original QBF as a formula in CNF. (Ijtihad; instrumented by Hadẑić,
    Bloem, Shukla, and Seidl in 2022.)
    """
    returncode, _, _, time_us = call_subprocess(
        args=(
            dependencies / DepNames.ijtihad,
            "--wit_per_call=-1",
            "--cex_per_call=-1",
            f"--tmp_dir={cnf.parent!s}/",
            f"--log_phi={cnf!s}",
            input_,
        ),
        capture_stdout=True,
        capture_stderr=True,
        capture_color_stdout=OTHER_STDOUT,
        capture_color_stderr=OTHER_STDERR,
        expected_exit={10, 20},
    )
    try:
        if returncode == 10:
            fatal(
                ExitCode.QBF_SAT, f"{style(BAD, 'QBF is SAT')}, cannot proceed"
            )
        status(style(GOOD, "QBF is UNSAT"))
    finally:
        end_profile(ProfileNames.QBF_SOLVE, int(time_us), no_start=True)

#  }}}

#  gen_rat_proof {{{
#  ~~~~~~~~~~~~~~~~~~~~ gen_rat_proof ~~~~~~~~~~~~~~~~~~~~
@status_function
def gen_rat_proof(
    dependencies: Dependencies,
    cnf: Path,
    rat: Path,
    lrat: bool,
) -> None:
    """
    Calling a SAT solver on the obtained expansion clauses in CNF to generate a
    RAT proof of the formula. (SAT solver Kissat by Biere, Fazekas, Fleury, and
    Heisinger, 2020.)
    """
    # TODO: Update doc-string for LRAT
    returncode, _, _, time_us = call_subprocess(
        args=(
            dependencies / DepNames.cadical,
            *(() if get_show_color() else ("--no-colors",)),
            "--unsat",
            "--no-binary",
            "--lrat",
            "-q",
            cnf,
            rat,
        ) if lrat else ( # Faster, but cannot produce LRAT traces... yet.
            dependencies / DepNames.kissat,
            *(() if get_show_color() else ("--no-colors",)),
            "--unsat",
            "--no-binary",
            "-q",
            cnf,
            rat,
        ),
        capture_stdout=True,
        capture_stderr=True,
        capture_color_stdout=OTHER_STDOUT,
        capture_color_stderr=OTHER_STDERR,
        expected_exit={10, 20},
    )
    try:
        if returncode == 10:
            fatal(
                ExitCode.EXPANSION_SAT,
                f"{style(BAD, 'CNF expansion is SAT')}, cannot proceed"
            )
        status(style(GOOD, "CNF expansion is UNSAT"))
    finally:
        end_profile(ProfileNames.GEN_RAT_PROOF, int(time_us), no_start=True)

#  }}}

#  gen_ferat_proof {{{
#  ~~~~~~~~~~~~~~~~~~~~ gen_ferat_proof ~~~~~~~~~~~~~~~~~~~~
@status_function
def gen_ferat_proof(
    dependencies: Dependencies,
    cnf: Path,
    rat: Path,
    output: Path,
) -> None:
    """
    Merging the expansion of the QBF solver with the RAT proof of the SAT solver
    to create a \\forall-Exp+RAT (FERAT) proof.
    """
    start_time_us = start_profile()
    try:
        with open(output, "w", **ENCODING) as output_file:
            # First, comment all lines of the CNF expansion and write it to the
            # FERAT output
            with open_zip_agnostic(cnf, "r") as cnf_file:
                for cnf_line in cnf_file.readlines():
                    if RE_DIMACS_LINE("p").match(cnf_line): continue
                    match_ = RE_DIMACS_LINE("c", "([xo].*)$").match(cnf_line)
                    if match_ is None:
                        if RE_DIMACS_LINE("c").match(cnf_line):
                            # Regular comment, just copied over
                            output_file.write(cnf_line)
                            continue
                        # Regular clause, turned into 'ex' line
                        output_file.write(f"e {cnf_line!s}")
                    else:
                        # 'c x' or 'c o' header turned into just 'x' and 'o'
                        output_file.write(f"{match_[1]!s}\n")
            # Second, read the RAT proof and write it to the FERAT file
            # directly, so that drat-trim can process it as it is
            with open_zip_agnostic(rat, "r") as drat_file:
                output_file.write(drat_file.read())
        status(f"Generated FERAT proof")
    finally:
        end_profile(ProfileNames.GEN_FERAT_PROOF, start_time_us)

#  }}}

#  ~~~~~~~~~~~~~~~~~~~~ Check ONLY ~~~~~~~~~~~~~~~~~~~~

#  split_ferat {{{
#  ~~~~~~~~~~~~~~~~~~~~ split_ferat ~~~~~~~~~~~~~~~~~~~~
@status_function
def split_ferat(ferat: Path, cnf: Path, rat: Path) -> None:
    """ Splitting the FERAT proof into its CNF and RAT components. """
    start_time_us = start_profile()
    try:
        with (
            open_zip_agnostic(ferat, "r") as ferat_file,
            open(cnf, "w", **ENCODING) as cnf_file,
            open(rat, "w", **ENCODING) as rat_file,
        ):
            max_var = num_clauses = insert_p_header_before = 0
            for i, ferat_line in enumerate(ferat_file.readlines()):
                match_ = RE_DIMACS_LINE("e", "(.*)$").match(ferat_line)
                if match_ is None:
                    # IMPORTANT: We need to match as little as possible to only
                    #            get the first part of the
                    #            'c x <exp_vars> 0 <qbf_vars> 0 <annots> 0'
                    match_ = RE_DIMACS_LINE("x", "(.*?)", "0",
                                            ".*$").match(ferat_line)
                    if match_ is None: continue
                else:
                    # TODO: The 'p cnf ...' header has to come after the 'c x'
                    #       and 'c o' comments, unfortunately. Maybe a fix for
                    #       this would be nice in the future..?
                    if insert_p_header_before < 1: insert_p_header_before = i
                    num_clauses = num_clauses + 1
                # Capture group 1 must always contain a space-delimited list of
                # variables
                max_var = max(
                    (max_var, *(abs(int(num)) for num in match_[1].split()))
                )
            ferat_file.seek(0, os.SEEK_SET)
            for i, ferat_line in enumerate(ferat_file.readlines()):
                match_ = RE_DIMACS_LINE("([eox])", "(.*)$").match(ferat_line)
                # See comment above for 'p cnf ...' header
                if i == insert_p_header_before:
                    cnf_file.write(f"p cnf {max_var!s} {num_clauses!s}\n")
                # RAT line
                if match_ is None:
                    rat_file.write(ferat_line)
                # All others here are part of the expansion, indices are
                # capture groups
                elif match_[1] == "e":
                    cnf_file.write(f"{match_[2]!s}\n")
                else:
                    cnf_file.write(f"c {match_[1]!s} {match_[2]!s}\n")
        status(f"Split FERAT proof")
    finally:
        end_profile(ProfileNames.SPLIT_FERAT, start_time_us)

#  }}}

#  }}}

#  CLI {{{
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ CLI ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

def cli(parser: ArgumentParser) -> Namespace:
    #  General Options {{{
    #  ~~~~~~~~~~~~~~~~~~~~ General Options ~~~~~~~~~~~~~~~~~~~~
    # Mode group {{{
    mode_group = parser.add_argument_group(title="mode")
    mode_group.add_argument(
        "-l",
        "--lrat",
        help="use LRAT instead of (D)RAT in the pipeline" \
             " (default = False)",
        action=BooleanOptionalAction,
        default=False,
        dest="lrat",
    )
    # }}}
    # Verbosity group {{{
    verbosity_group = parser.add_argument_group(title="verbosity")
    verbosity_group.add_argument(
        "-c",
        "--show-command",
        help="shows the output of all commands invoked by the pipeline" \
             " (default = False)",
        action=BooleanOptionalAction,
        default=False,
        dest="show_command",
    )
    verbosity_group.add_argument(
        "-e",
        "--explain",
        help="shows the explanation of each function called by the pipeline" \
             " (default = True)",
        action=BooleanOptionalAction,
        default=True,
        dest="explain",
    )
    verbosity_group.add_argument(
        "-V",
        "--verbose",
        help="produces all output (implies --show-command and --explain)",
        default=False,
        action="store_true",
        dest="verbose",
    )
    verbosity_group.add_argument(
        "-q",
        "--quiet",
        help="produces the least output (implies --no-show-command and"
        " --no-explain)",
        default=False,
        action="store_true",
        dest="quiet",
    )
    # }}}
    # Misc group {{{
    misc_group = parser.add_argument_group(title="misc")
    misc_group.add_argument(
        "-t",
        "--timeout",
        help="specifies a timeout duration in seconds, or no timeout if set" \
             " to 0 (default = 0)",
        default=0.0,
        type=float,
        dest="timeout",
    )
    misc_group.add_argument(
        "-K",
        "--keep-tmp",
        help="specifies not to delete the temporary directory after pipeline" \
             " is done (defaults = False)",
        action=BooleanOptionalAction,
        default=False,
        dest="keep_tmp",
    )
    misc_group.add_argument(
        "-T",
        "--tmp",
        help="sets the temporary directory (default = ./tmp)",
        default=Path("./tmp"),
        type=Path,
        dest="tmp_dir",
    )
    deps_path = Path(__file__).with_name("bin")
    misc_group.add_argument(
        "-d",
        "--deps",
        help="sets the path to the built dependencies"
        f" (default = ./{deps_path.relative_to(Path.home())!s})",
        type=Path,
        dest="deps_dir",
        default=deps_path,
    )
    misc_group.add_argument(
        "-C",
        "--color",
        help="enables the ANSI escape sequences for colored output" \
             " (default = auto)",
        choices={"never", "auto", "always"},
        default="auto",
        dest="color",
    )
    misc_group.add_argument(
        "--profile",
        help="displays the execution time in microseconds for each step" \
             " (default = False)",
        action=BooleanOptionalAction,
        default=False,
        dest="profile",
    )
    # }}}
    #  }}}

    # subparsers
    subparsers = parser.add_subparsers(dest="command", required=True)

    #  Version Subparser {{{
    #  ~~~~~~~~~~~~~~~~~~~~ Version Subparser ~~~~~~~~~~~~~~~~~~~~
    version_parser = subparsers.add_parser(
        Commands.VERSION, help="Print the version and exit"
    )
    #  }}}

    #  Generation Subparser {{{
    #  ~~~~~~~~~~~~~~~~~~~~ Generation Subparser ~~~~~~~~~~~~~~~~~~~~
    gen_parser = subparsers.add_parser(
        Commands.GENERATE, help="Generate a FERAT proof from a QBF"
    )
    gen_file_group = gen_parser.add_argument_group(title="files")
    gen_file_group.add_argument(
        "--expansion",
        help="if a QBF file, as well as an expansion of said file is" \
             " already available, it can be defined here, and the pipeline" \
             " expansion steps will be skipped",
        type=Path,
    )
    gen_file_group.add_argument(
        "input",
        help="sets the path to the input (QBF) file(s), when multiple files" \
             " are given, the output must be a folder, when the --expansion" \
             " flag is used, only one input may be provided",
        type=Path,
        nargs="+",
    )
    gen_file_group.add_argument(
        "output",
        help="sets the path to the output (FERAT proof) file, when a folder" \
             " is given, the name of the input file will be appended to it",
        type=Path,
    )
    #  }}}

    #  Check Subparser {{{
    #  ~~~~~~~~~~~~~~~~~~~~ Check Subparser ~~~~~~~~~~~~~~~~~~~~
    check_parser = subparsers.add_parser(
        Commands.CHECK, help="Check an existing FERAT proof with a given QBF"
    )
    check_file_group = check_parser.add_argument_group(title="files")
    check_file_group.add_argument(
        "qbf",
        help="sets the path to the input QBF file",
        type=Path,
    )
    check_file_group.add_argument(
        "ferat",
        help="sets the path to the input FERAT file",
        type=Path,
    )
    #  }}}

    # parse args, skipping program name
    try:
        return parser.parse_args(sys.argv[1:])
    except (ArgumentError, ArgumentTypeError) as args_err:
        fatal(ExitCode.CLI_ERR, args_err)

#  }}}

#  Main Entrypoint {{{
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~ Main Entrypoint ~~~~~~~~~~~~~~~~~~~~
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

def main() -> NoReturn:
    parser = ArgumentParser(
        description="The FERAT pipeline script. This pipeline will generate" \
                    " and verify a FERAT proof from an input QBF.",
        allow_abbrev=True,
    )
    args = cli(parser)

    # we need to pull out the 'keep_tmp' variable for the 'finally' clause of
    # this large try-except block, we set the value of 'keep_tmp' to True, so
    # that 'tmp_dir' is never accessed too early
    keep_tmp: bool = True
    tmp_dir: Path = Path("./tmp")
    start_time_us = start_profile()
    try:
        # get arguments into a nice form with types
        lrat: bool = args.lrat
        command: str = args.command
        show_command: bool = args.show_command
        explain: bool = args.explain
        verbose: bool = args.verbose
        quiet: bool = args.quiet
        timeout: float = args.timeout
        keep_tmp = args.keep_tmp
        tmp_dir = args.tmp_dir
        deps_dir: Path = args.deps_dir
        color: str = args.color
        profile: bool = args.profile

        # Set globals
        set_timeout(timeout)
        # TODO(marcel): This is a mess, lol
        set_show_command((not quiet) and (verbose or show_command))
        set_show_status((not quiet) and (verbose or explain))
        if color == "never": set_show_color(False)
        elif color == "auto": set_show_color(sys.stdout.isatty())
        elif color == "always": set_show_color(True)
        else: assert False, f"Invalid color mode {color!r}"
        set_profile(profile)

        # Before we do anything, potentially execute the version command
        if command == Commands.VERSION:
            print(
                f"{style(FERAT, 'FERAT')} by Simader, Seidl, and Rebola-Pardo"
            )
            print(f"Version {VERSION}")
            sys.exit(0)

        #  check general constraints
        if not (deps_dir.exists() and deps_dir.is_dir()):
            fatal(
                ExitCode.DEPS_NOT_FOUND,
                f"Unable to find binaries in given path ('{deps_dir!s}')."
                f"\nDid you run cmake? If not, follow these steps:"
                f"\n    `cmake -B build/ && make -C build/ && pip install .`"
            )

        #  Do path preparations
        if not tmp_dir.exists():
            os.makedirs(tmp_dir, mode=0o755, exist_ok=True)
        # Set up dependencies
        dependencies = init_dependencies(deps_dir, lrat)

        status(f"Command chosen is {style(IMPORTANT, command)}")

        #  Generate Command {{{
        #  ~~~~~~~~~~~~~~~~~~~~ Generate Command ~~~~~~~~~~~~~~~~~~~~
        if command == Commands.GENERATE:
            expansion: Path | None \
                = args.expansion if ("expansion" in args) else None
            qbfs: Sequence[Path] = args.input
            output: Path = args.output
            # If the command picked is 'generate', we can create and check the
            # RAT separately, and then combine it into FERAT to save some time

            # Make sure variable number of inputs is well-behaved
            num_in = len(qbfs)
            if num_in > 1:
                if expansion is not None:
                    fatal(
                        ExitCode.CLI_ERR,
                        f"When '--expansion' is specified ('{expansion!s}')"
                        f", only one input may be given, but {num_in} were"
                        f" supplied"
                    )
                if profile:
                    fatal(
                        ExitCode.CLI_ERR,
                        f"Profiling is only available when one input file is"
                        f" specified, but {num_in} were supplied",
                    )
                if not output.is_dir():
                    fatal(
                        ExitCode.CLI_ERR,
                        f"When specifying multiple input files, the output"
                        f"must be a folder, not a file ('{output!s}')"
                    )
            # If 'ouput' is a folder, use the input name(s)
            outputs: Sequence[Path]
            if output.is_dir():
                outputs = tuple(
                    output.joinpath(f"{i.stem}.ferat") for i in qbfs
                )
            else:
                outputs = (output,)

            for qbf, output in zip(qbfs, outputs):
                status("")
                status(f"Processing '{style(IMPORTANT, qbf)}'")

                # name tmp files
                out_stem = output.stem
                rat = tmp_dir / f"{out_stem!s}.rat"
                simple_cnf: Path = tmp_dir / f"{out_stem!s}-simplified.cnf"
                simple_rat: Path = tmp_dir / f"{out_stem!s}-simplified.rat"

                # if '--expansion' is given, we know that the provided file
                # *has* to be associated with this one input/output tuple
                if expansion is None:
                    cnf = tmp_dir / f"{out_stem!s}.cnf"
                    # solve QBF and generate CNF expansion
                    solve_qbf(dependencies, qbf, cnf)
                else:
                    status(f"Using given expansion")
                    cnf = expansion
                # Create and check RAT proof, simplify to RAT' and CNF' (as
                # minimal unsatisfiable core)
                gen_rat_proof(dependencies, cnf, rat, lrat)
                ferat_cnf, ferat_rat = check_rat_proof(
                    dependencies, cnf, rat, lrat, simple_cnf, simple_rat
                )
                # Input QBF and CNF' to check expansion step
                check_expansion(dependencies, qbf, ferat_cnf)
                # Generate FERAT proof, which is a mix of CNF' and (d)RAT'
                gen_ferat_proof(dependencies, ferat_cnf, ferat_rat, output)
        #  }}}
        #  Check Command {{{
        #  ~~~~~~~~~~~~~~~~~~~~ Check Command ~~~~~~~~~~~~~~~~~~~~
        elif command == Commands.CHECK:
            qbf = args.qbf # already typed
            ferat: Path = args.ferat
            # If the command picked is 'check', we skip all steps that
            # produce the FERAT proof and only check the RAT properties and
            # the expansion
            cnf_comp = tmp_dir / f"{qbf.stem!s}-fsplit.cnf"
            rat_comp = tmp_dir / f"{qbf.stem!s}-fsplit.rat"
            split_ferat(ferat, cnf_comp, rat_comp)
            check_rat_proof(dependencies, cnf_comp, rat_comp, lrat)
            check_expansion(dependencies, qbf, cnf_comp)
        #  }}}

    except FERATFatalError as ferr:
        ferr.exit()
    finally:
        end_profile(ProfileNames.TOTAL, start_time_us)
        if (not keep_tmp) and tmp_dir.is_dir(): shutil.rmtree(tmp_dir)

    # Success!
    sys.exit(0)

if __name__ == "__main__":
    main()

#  }}}
# vim: foldmethod=marker
