/-!
  # L2 — oracle congruence (Paper A P-5 under P-1)

  Abstract compiler depends on decisions only through `ask`.
  Pointwise equal oracles ⇒ equal outputs. Instantiates:
  P-3 (`net ≡ gold` on keys) + P-1 ⇒ P-5 (`C[net] ≡ C[gold]`).
-/

namespace UnisaFormal.L2

/-- Opaque stage / key / class names as strings in the contract model. -/
abbrev Stage := String
abbrev Key := String
abbrev Class := String

/-- Oracle: stage × key → class (prd `ask`). -/
abbrev Oracle := Stage → Key → Class

/--
  Abstract compiler: a function of an oracle.
  Real walkers/lowering are inhabitants; we only need extensionality in `ask`.
-/
abbrev Compiler (Output : Type) := Oracle → Output

/-- P-5 congruence: if two oracles agree everywhere, compiler outputs agree. -/
theorem cong_of_pointwise {Output : Type} (C : Compiler Output) (O₁ O₂ : Oracle)
    (h : ∀ s k, O₁ s k = O₂ s k) : C O₁ = C O₂ := by
  have : O₁ = O₂ := funext (fun s => funext (fun k => h s k))
  rw [this]

/--
  Under the model that `C` is applied to the *restriction* of an oracle to a
  known key set `asks`, agreement on that set suffices.
-/
theorem cong_on_asks {Output : Type} (C : Compiler Output)
    (wrap : Oracle → Oracle) (O₁ O₂ : Oracle)
    (hwrap : ∀ O O', (∀ s k, O s k = O' s k) → wrap O = wrap O')
    (h : ∀ s k, O₁ s k = O₂ s k) : C (wrap O₁) = C (wrap O₂) := by
  have : wrap O₁ = wrap O₂ := hwrap O₁ O₂ h
  rw [this]

/--
  P-3 bridge (schematic): if `net` and `gold` agree on every concrete ask the
  walker issues, congruence lifts to equal compiles. The list `asked` is the
  finite witness a twin-test / trace would record.
-/
theorem p5_of_p3_trace {Output : Type} (C : Compiler Output)
    (net gold : Oracle) (asked : List (Stage × Key))
    (htrace : ∀ p ∈ asked, net p.1 p.2 = gold p.1 p.2)
    (hdep : ∀ O O', (∀ p ∈ asked, O p.1 p.2 = O' p.1 p.2) → C O = C O') :
    C net = C gold :=
  hdep net gold htrace

end UnisaFormal.L2
