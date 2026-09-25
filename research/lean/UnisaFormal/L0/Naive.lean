/-!
  # L0 — naive per-key unit construction (Paper A §3.1 / prd P-8)

  Restates a known encoding (lookup / Tracr-style): one hidden unit per key.
  Not claimed as a novel theorem — machine-checked citation material for A.
-/

namespace UnisaFormal.L0

/--
  Scores for class `y` under the naive net for gold `G : Fin nK → Fin nY`.

  Integer model of §3.1: after one-hot embed + per-key unit + ReLU, logits are
  the indicator `1{G k = y}`.
-/
def naiveLogit {nK nY : Nat} (G : Fin nK → Fin nY) (k : Fin nK) (y : Fin nY) : Int :=
  if G k = y then 1 else 0

theorem naive_logit_eq_one_iff {nK nY : Nat} (G : Fin nK → Fin nY) (k : Fin nK)
    (y : Fin nY) : naiveLogit G k y = 1 ↔ y = G k := by
  unfold naiveLogit
  constructor
  · intro h
    split at h
    · next heq => exact heq.symm
    · next => cases h
  · intro h
    simp [h]

theorem naive_logit_eq_zero_iff {nK nY : Nat} (G : Fin nK → Fin nY) (k : Fin nK)
    (y : Fin nY) : naiveLogit G k y = 0 ↔ y ≠ G k := by
  unfold naiveLogit
  constructor
  · intro h
    split at h
    · next => cases h
    · next hne => exact Ne.symm hne
  · intro hne
    have : G k ≠ y := Ne.symm hne
    simp [this]

/--
  Strict argmax witness on an indicator that peaks uniquely at `ystar` with
  value 1 and is 0 elsewhere. Evaluating the net is returning that witness.
-/
def pickUnique {nY : Nat} (ystar : Fin nY) (score : Fin nY → Int)
    (_h1 : score ystar = 1)
    (_h0 : ∀ y, y ≠ ystar → score y = 0) : Fin nY :=
  ystar

theorem pickUnique_eq {nY : Nat} (ystar : Fin nY) (score : Fin nY → Int)
    (h1 : score ystar = 1) (h0 : ∀ y, y ≠ ystar → score y = 0) :
    pickUnique ystar score h1 h0 = ystar := rfl

/-- Network prediction recovers gold at every key (P-8 naive / §3.1). -/
def naiveEval {nK nY : Nat} (G : Fin nK → Fin nY) (k : Fin nK) : Fin nY :=
  pickUnique (G k) (naiveLogit G k)
    (by simp [naiveLogit])
    (by
      intro y hy
      have : G k ≠ y := Ne.symm hy
      simp [naiveLogit, this])

theorem naive_exact {nK nY : Nat} (G : Fin nK → Fin nY) (k : Fin nK) :
    naiveEval G k = G k := by
  rfl

theorem naive_logit_le_one {nK nY : Nat} (G : Fin nK → Fin nY) (k : Fin nK)
    (y : Fin nY) : naiveLogit G k y ≤ 1 := by
  unfold naiveLogit; split <;> omega

theorem naive_logit_nonneg {nK nY : Nat} (G : Fin nK → Fin nY) (k : Fin nK)
    (y : Fin nY) : 0 ≤ naiveLogit G k y := by
  unfold naiveLogit; split <;> omega

end UnisaFormal.L0
