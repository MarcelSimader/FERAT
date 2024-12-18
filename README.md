# FERAT

FERAT (an acronym for \forall-Exp+RAT) is a clausal, interference proof system for
expansion-based QBF solvers, utilizing (D)RAT as crucial new component. This differs from
its predecessor, which used the less powerful resolution (Res) calculus.

## How to Build

Before the Python package can be installed, the appropriate solver and checker backends
must be compiled for your system. The project ships with a CMake file.

To build all the backends, run the following command (the `-j` flag optionally uses
parallel builds, which are already automatically used in the subprojects):
```sh
cmake -B build/ && make -C build/ [-j]
```

## How to Use

> NOTE:
> The `ferat` package is intended for newer versions of Python: A virtual environment with
> Python version `>=3.10` is *highy recommended*.

### Installing Globally

On some systems, and some versions of `pip`, one may simply run the same command as below,
outside of a virtual environment.

Newer versions of `pip` will **not** allow you to do this! You can use a program like
`pipx` (for Ubuntu: `apt install pipx`), which manages globally installed packages. Simply
run `pipx install .`.

Optionally, supply the `--user` flag to install to the user site packages, and supply the
`--editable` (`-e`) flag to install an editable version for development. All of the same
flags work for `pipx`.

### Using Python Virtual Environments

First, create a virtual environment and activate it.

Next, install the project into the virtual environment (add `-e` flag if you plan on
editing the code):
```sh
pip install [-e] .
```

---

Now, it is possible to run the project using the script entry point `ferat`, or
`forall-exp-rat`, or `python -m ferat`, or the provided `ferat.py` file.

```sh
# For more help, see
ferat --help
# or for a simple example, here is generating
ferat generate "path/to/some/qbf.qdimacs" "our_proof.ferat"
# and here is checking.
ferat check "path/to/some/other-qbf.qdimacs" "existing_proof.ferat"
```

## License

See file `LICENSE`.
