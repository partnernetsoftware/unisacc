import UnisaFormal.L2.Congruence

/-!
  # L3 — domain closure / P-2 sample (`reloc`)

  Gold stage `reloc` (`unisa/gold.py`): keys are the full product
  `JMPKIND × ARCH` with `JMPKIND = (jmp, jz, call)` and
  `ARCH = (x86_64, arm64)` — six keys; labels in `RELKIND`.

  This file is a *schematic* machine-checked P-2: an abstract walker
  projects ask keys from a tiny program structure into that product;
  every emitted key lies in the gold domain `K`. Optionally lifts to
  L2 congruence when `net ≡ gold` on `K`.
-/

namespace UnisaFormal.L3

/-! ## Reloc key space (hardcoded Fin / enum model of gold.py) -/

/-- Mirrors `JMPKIND = ("jmp", "jz", "call")`. -/
inductive Kind where
  | jmp
  | jz
  | call
  deriving DecidableEq, Repr

/-- Mirrors `catalog.ARCH = ("x86_64", "arm64")`. -/
inductive Arch where
  | x86_64
  | arm64
  deriving DecidableEq, Repr

/-- Mirrors `RELKIND = ("rel32", "arm26", "arm19")` (label space; unused in P-2). -/
inductive RelKind where
  | rel32
  | arm26
  | arm19
  deriving DecidableEq, Repr

/-- Stage key = `kind × arch` (gold input fields). -/
abbrev Key := Kind × Arch

/-- Gold table domain: full product (6 keys). -/
def K : List Key :=
  [ (Kind.jmp, Arch.x86_64), (Kind.jmp, Arch.arm64)
  , (Kind.jz, Arch.x86_64),  (Kind.jz, Arch.arm64)
  , (Kind.call, Arch.x86_64), (Kind.call, Arch.arm64) ]

theorem mem_K_of_key (k : Key) : k ∈ K := by
  rcases k with ⟨kind, arch⟩
  cases kind <;> cases arch <;> decide

/-! ## Abstract walker: keys projected from structure -/

/-- Tiny reloc-relevant site: a jump/call kind (arch is the compile target). -/
structure Site where
  kind : Kind
  deriving Repr

/--
  Program fragment the walker walks: an ordered list of reloc sites under a
  fixed target architecture. Real lowering projects `(kind, arch)` asks the
  same way; we do not model IR, only the key projection.
-/
structure Prog where
  arch : Arch
  sites : List Site
  deriving Repr

/-- Ask keys the abstract walker emits (one per site). -/
def asks (p : Prog) : List Key :=
  p.sites.map fun s => (s.kind, p.arch)

/-- P-2 for this toy: every key the walker issues is in the gold domain `K`. -/
theorem asks_subset_K (p : Prog) : ∀ k ∈ asks p, k ∈ K := by
  intro k hk
  simp only [asks, List.mem_map] at hk
  obtain ⟨s, _, rfl⟩ := hk
  exact mem_K_of_key (s.kind, p.arch)

/-- Same fact as membership in the finite list witness. -/
theorem asks_mem_K (p : Prog) (k : Key) (hk : k ∈ asks p) : k ∈ K :=
  asks_subset_K p k hk

/-! ## Optional bridge: P-2 + pointwise net≡gold on K ⇒ L2 on asks -/

open UnisaFormal.L2

/-- String names matching gold.py vocab (L2 oracle key). -/
def kindStr : Kind → String
  | .jmp => "jmp"
  | .jz => "jz"
  | .call => "call"

def archStr : Arch → String
  | .x86_64 => "x86_64"
  | .arm64 => "arm64"

/-- Encode a reloc key as the L2 string contract key `"kind/arch"`. -/
def encodeKey (k : Key) : String :=
  kindStr k.1 ++ "/" ++ archStr k.2

/-- Trace of asks as L2 `(stage, key)` pairs. -/
def askedTrace (p : Prog) : List (Stage × String) :=
  (asks p).map fun k => ("reloc", encodeKey k)

/--
  If `net` and `gold` agree on every key in the gold domain `K`, they agree
  on every ask the walker issues (P-2 + domain coverage ⇒ trace equality).
-/
theorem net_eq_gold_on_asks (p : Prog) (net gold : Oracle)
    (hK : ∀ k ∈ K, net "reloc" (encodeKey k) = gold "reloc" (encodeKey k)) :
    ∀ q ∈ askedTrace p, net q.1 q.2 = gold q.1 q.2 := by
  intro q hq
  simp only [askedTrace, List.mem_map] at hq
  obtain ⟨k, hk, rfl⟩ := hq
  have hkK : k ∈ K := asks_subset_K p k hk
  exact hK k hkK

/--
  Schematic P-5 for one reloc walk: agreement on `K` + asks ⊆ `K` +
  compiler dependence only on the ask trace ⇒ `C[net] = C[gold]`.
-/
theorem p5_reloc_of_domain_closure {Output : Type} (C : Compiler Output)
    (p : Prog) (net gold : Oracle)
    (hK : ∀ k ∈ K, net "reloc" (encodeKey k) = gold "reloc" (encodeKey k))
    (hdep : ∀ O O', (∀ q ∈ askedTrace p, O q.1 q.2 = O' q.1 q.2) → C O = C O') :
    C net = C gold :=
  p5_of_p3_trace C net gold (askedTrace p) (net_eq_gold_on_asks p net gold hK) hdep

end UnisaFormal.L3
