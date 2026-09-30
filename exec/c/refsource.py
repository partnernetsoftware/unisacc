"""Build the scratch reference source from the ordered unisacc.c manifest."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def source_text(root=ROOT):
    """Use the same include expansion as tests/build_ref.sh."""
    with tempfile.TemporaryDirectory(prefix='unisacc-refsource-') as td:
        flat = Path(td) / 'ref.c'
        subprocess.run([str(root / 'tests/export_ref.sh'), str(flat)],
                       check=True, capture_output=True, timeout=30)
        return ((root / 'tests/refshim.h').read_text()
                + flat.read_text()
                + (root / 'tests/reffoot.h').read_text())


def compile_command(root, source, output):
    return ['cc', '-w', '-O1', '-I', root / 'kernel', '-I', root / 'src',
            source, '-o', output]
