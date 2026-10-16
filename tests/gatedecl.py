#!/usr/bin/env python3
"""Shared gate declaration semantics (gatedeps.json): one place for the two readings the tools already had.

effective_entry(manifest, name) is the QUEUE reading (gatequeue's fingerprint input): a suite declared with a family
takes the family's inputs whole, keeps its own command and the family name.  Identity-bearing: it must not change.

declared_reasons(entry, families, path) is the SELECTION reading (affected, commitgate's merged judgment): the suite's
own declared inputs plus the family's reviewed hits (a union, conservative).  It only decides what to consider,
never a result."""
import pathlib

PRODUCT = ('exec/', 'unisa/', 'src/', 'kernel/', 'include/', 'weights/', 'seed/')
PRODUCT_FILES = ('unisacc.c',)


def is_partial(entry):
    """Partial evidence adds direct hits; it never discharges global obligations."""
    if entry is None:
        return False
    if 'partial' in entry and type(entry['partial']) is not bool:
        raise ValueError('declaration-partial-must-be-boolean')
    return entry.get('partial', False)


def effective_entry(manifest, name):
    entry = manifest.get('suites', {}).get(name)
    if is_partial(entry):
        return None  # Preserve the queue's undeclared/global fingerprint reading.
    if entry and 'family' in entry:
        entry = dict(manifest['families'][entry['family']], command=entry['command'], family=entry['family'])
    return entry


def in_tree(path, tree):
    tree = tree.rstrip('/')
    return path == tree or path.startswith(tree + '/')


def family_hit(fam, path):
    trees = fam.get('reviewed_trees') or {}
    for t in trees:
        if not in_tree(path, t):
            continue
        if path in (fam.get('files') or []):
            return True
        suffix = pathlib.PurePosixPath(path).suffix
        if t in (fam.get('all_files_trees') or []) or suffix in (fam.get('inventory_suffixes') or []):
            if not any(in_tree(path, x) for x in (fam.get('excluded_dirs') or [])):
                return True
    return path in (fam.get('guards') or {}) or path in (fam.get('files') or [])


def declared_reasons(entry, families, path):
    out = ['global'] if is_partial(entry) else []
    if path in (entry.get('files') or []): out.append('files')
    if any(in_tree(path, t) for t in (entry.get('trees') or [])): out.append('trees')
    if path in (entry.get('guards') or {}): out.append('guard')
    if entry.get('executable_inputs') and (path.startswith(PRODUCT) or path in PRODUCT_FILES): out.append('product')
    fam = families.get(entry.get('family')) if entry.get('family') else None
    if fam and family_hit(fam, path): out.append('family')
    return out
