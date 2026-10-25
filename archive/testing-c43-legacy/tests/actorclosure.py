"""Shared conservative repository actor dependency closure; no subprocess."""
def actor_closure(root,seeds):
 """Conservative recursive local imports and root-relative shell/helper references.

 Generated SELF/generation binaries are bound separately; product dynamic trees
 remain covered by provenance.source_digest. No general shell execution parser.
 """
 import ast,re,hashlib,pathlib
 ROOT=pathlib.Path(root).resolve()
 digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
 pending=list(seeds);found={};edges={}
 searches=[ROOT/'tests',ROOT/'exec/c',ROOT/'exec/pipeline',ROOT]
 while pending:
  name=pending.pop()
  if name in found:continue
  path=ROOT/name;data=path.read_bytes();found[name]=digest(path);children=set()
  def local(candidate):
   candidate=candidate.resolve();candidate.relative_to(ROOT)
   try:info=candidate.stat()
   except FileNotFoundError:return
   if candidate.is_file():children.add(candidate.relative_to(ROOT).as_posix())
  text=data.decode()
  if path.suffix=='.py':
   for node in ast.walk(ast.parse(text)):
    names=[x.name for x in node.names] if isinstance(node,ast.Import) else [node.module] if isinstance(node,ast.ImportFrom) and node.module else []
    for module in names:
     for base in [path.parent,*searches]:
      target=pathlib.Path(*module.split('.'))
      local(base/target.with_suffix('.py'));local(base/target/'__init__.py')
   # Literal tests paths used by Python producers (export/exec/compile inputs).
   for ref in re.findall(r"['\"](tests/[A-Za-z0-9_./-]+)['\"]",text):local(ROOT/ref)
  elif path.suffix=='.sh' or path.name=='bound':
   for variable,ref in re.findall(r"\$(?:\{)?(R|ROOT|_LIB_R|here)(?:\})?(?:[\"'])?/([A-Za-z0-9_./-]+)",text):
    local((path.parent if variable=='here' else ROOT)/ref)
  elif path.suffix in ('.c','.h'):
   for ref in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',text,re.M):local(path.parent/ref)
  edges[name]=sorted(children);pending.extend(children)
 return dict(sorted(found.items())),dict(sorted(edges.items()))
