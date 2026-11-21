"""Compatibility entry point: replay all lessons and expose Part 4's checkpoints.

New callers should use verify_tutorials.py. Existing --output/sectionN paths
remain available; the complete replay lives under --output/all-tutorials.
"""

import argparse
from pathlib import Path
import shutil

try:
    from .verify_tutorials import replay, safe_output
except ImportError:
    from verify_tutorials import replay, safe_output


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build", action="store_true")
    parser.add_argument("--sdk", type=Path)
    parser.add_argument("--make", default="make")
    parser.add_argument("--gcc")
    args = parser.parse_args(argv)
    if args.build and not args.sdk:
        parser.error("--build requires --sdk")
    output = safe_output(args.output)
    if any((output / f"section{stage}").exists() for stage in range(1, 8)):
        parser.error("Part 4 checkpoint output already exists; choose a fresh --output")
    replay(output / "all-tutorials", args.build, args.sdk, args.make, args.gcc)
    for stage in range(1, 8):
        shutil.copytree(output / "all-tutorials" / "part4" / f"section{stage}",
                        output / f"section{stage}")
    print("PASS: compatible Part 4 checkpoints available at OUTPUT/section1..section7")


if __name__ == "__main__":
    main()
