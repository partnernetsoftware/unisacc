/-!
  # L1 — cube / decision-list soundness (Paper A / construct.py §T2)

  Simplified model of the *shipping* construction story in `unisa/construct.py`:

  * **Cube** = per-field optional match (don't-care vs concrete value / small set).
    Membership on a flat key space `Fin nK` is the oracle the DL evaluates;
    field-wise literals build that oracle (`prodHit`), without porting EXPAND /
    REDUCE / greedy cover.
  * **Decision list** = ordered `List` of `(cube, label)`. Semantics = **first
    match wins** (Rivest DL). This matches `decision_list`'s highest-priority-first
    rule order. The deployed net realises the same total order with positive
    `W2 = 2^rank` scores (`ranks` / `rep_from_dl`); under a correct ranking,
    unique argmax ≡ first match. We formalise the DL reading (no ties by
    construction).

  Soundness: if every key is covered and every rule is monochrome w.r.t. gold `G`
  (fires only on keys labelled `G k`), then `eval? = some (G k)` everywhere.
-/

namespace UnisaFormal.L1

/-! ## Field literals and product keys -/

/--
  Literal on one field of vocabulary size `nV`.

  * `star` — don't-care (`S_i` = full vocab in construct.py; no bias literal).
  * `set allowed` — set-valued literal (T1); singleton = concrete value.
-/
inductive Lit (nV : Nat) where
  | star
  | set (allowed : List (Fin nV))
  deriving Repr

/-- Does field value `v` satisfy the literal? -/
def Lit.hit {nV : Nat} : Lit nV → Fin nV → Bool
  | .star, _ => true
  | .set allowed, v => allowed.contains v

/-- Product key: `m` fields, each `Fin nV`. -/
abbrev ProdKey (m nV : Nat) := Fin m → Fin nV

/-- Conjunction of per-field literals (cube algebra, product form). -/
def prodHit {m nV : Nat} (lits : Fin m → Lit nV) (k : ProdKey m nV) : Bool :=
  (List.range m).all fun i =>
    if h : i < m then (lits ⟨i, h⟩).hit (k ⟨i, h⟩) else true

theorem lit_star_hit {nV : Nat} (v : Fin nV) :
    Lit.hit (.star : Lit nV) v = true := rfl

theorem lit_set_hit_eq {nV : Nat} (allowed : List (Fin nV)) (v : Fin nV) :
    Lit.hit (.set allowed) v = allowed.contains v := rfl

theorem prodHit_all_star {m nV : Nat} (k : ProdKey m nV) :
    prodHit (fun _ => Lit.star) k = true := by
  simp [prodHit, Lit.hit]

/-! ## Flat cubes and decision lists -/

/--
  Cube on the flat domain `Fin nK` (construct.py `Domain.n` after quotient).
  Field-wise cubes induce this via any encoding of `ProdKey → Fin nK`; soundness
  only needs the Boolean membership oracle (`hit`).
-/
structure Cube (nK : Nat) where
  hit : Fin nK → Bool

/-- One decision-list term. -/
structure Rule (nK nY : Nat) where
  cube : Cube nK
  label : Fin nY

/-- Rivest decision list: head = highest priority. -/
abbrev DecisionList (nK nY : Nat) := List (Rule nK nY)

/-- First matching rule's label, if any. -/
def eval? {nK nY : Nat} (rs : DecisionList nK nY) (k : Fin nK) : Option (Fin nY) :=
  match rs with
  | [] => none
  | r :: rest => if r.cube.hit k then some r.label else eval? rest k

/-- Every key fires at least one rule. -/
def Covers {nK nY : Nat} (rs : DecisionList nK nY) : Prop :=
  ∀ k : Fin nK, ∃ r ∈ rs, r.cube.hit k = true

/--
  Every rule is consistent with gold on its extent (monochrome cube).
  Stronger than “only the first hit is correct”, and the natural invariant of
  `expand` / `decision_list` (a term never claims a remaining key of another class;
  already-covered keys may be shadowed — first-match handles that).
-/
def Monochrome {nK nY : Nat} (rs : DecisionList nK nY) (G : Fin nK → Fin nY) : Prop :=
  ∀ r ∈ rs, ∀ k : Fin nK, r.cube.hit k = true → r.label = G k

theorem monochrome_cons {nK nY : Nat} (r0 : Rule nK nY) (rest : DecisionList nK nY)
    (G : Fin nK → Fin nY) (hM : Monochrome (r0 :: rest) G) :
    Monochrome rest G :=
  fun r hr k hm => hM r (List.mem_cons_of_mem r0 hr) k hm

/-- A concrete matching rule in the list ⇒ `eval?` is `some`. -/
theorem eval?_isSome_of_mem {nK nY : Nat} :
    ∀ (rs : DecisionList nK nY) (k : Fin nK) (r : Rule nK nY),
      r ∈ rs → r.cube.hit k = true → (eval? rs k).isSome = true
  | [], _, _, hr, _ => by cases hr
  | r0 :: rest, k, r, hr, hm => by
    simp only [eval?]
    cases hr with
    | head => simp [hm]
    | tail _ hrest =>
      cases h : r0.cube.hit k with
      | true => simp
      | false => exact eval?_isSome_of_mem rest k r hrest hm

/-- If `eval?` is defined at `k`, some list member hits `k`. -/
theorem exists_mem_of_eval?_isSome {nK nY : Nat} :
    ∀ (rs : DecisionList nK nY) (k : Fin nK),
      (eval? rs k).isSome = true → ∃ r ∈ rs, r.cube.hit k = true
  | [], k, h => by simp [eval?] at h
  | r0 :: rest, k, h => by
    simp only [eval?] at h
    cases hhit : r0.cube.hit k with
    | true =>
      refine ⟨r0, List.mem_cons_self r0 rest, hhit⟩
    | false =>
      simp only [hhit] at h
      obtain ⟨r, hr, hm⟩ := exists_mem_of_eval?_isSome rest k h
      exact ⟨r, List.mem_cons_of_mem r0 hr, hm⟩

theorem eval?_isSome_of_covers {nK nY : Nat} (rs : DecisionList nK nY)
    (hC : Covers rs) (k : Fin nK) : (eval? rs k).isSome = true := by
  obtain ⟨r, hr, hm⟩ := hC k
  exact eval?_isSome_of_mem rs k r hr hm

theorem covers_iff_eval?_isSome {nK nY : Nat} (rs : DecisionList nK nY) :
    Covers rs ↔ ∀ k : Fin nK, (eval? rs k).isSome = true :=
  ⟨fun hC k => eval?_isSome_of_covers rs hC k,
   fun h k => exists_mem_of_eval?_isSome rs k (h k)⟩

/--
  Helper: given a membership witness, first-match recovers gold under monochrome.
-/
theorem eval?_eq_gold_of_mem_monochrome {nK nY : Nat} :
    ∀ (rs : DecisionList nK nY) (G : Fin nK → Fin nY) (k : Fin nK)
      (w : Rule nK nY),
      w ∈ rs → w.cube.hit k = true → Monochrome rs G →
      eval? rs k = some (G k)
  | [], _, _, _, hw, _, _ => by cases hw
  | r0 :: rest, G, k, w, hw, hmw, hM => by
    unfold eval?
    by_cases h0 : r0.cube.hit k = true
    · have hlab : r0.label = G k := hM r0 (List.mem_cons_self r0 rest) k h0
      simp [h0, hlab]
    · have hw' : w ∈ rest := by
        cases hw with
        | head => exact False.elim (h0 hmw)
        | tail _ h => exact h
      have hM' : Monochrome rest G := monochrome_cons r0 rest G hM
      simp [h0]
      exact eval?_eq_gold_of_mem_monochrome rest G k w hw' hmw hM'

/--
  Core soundness (P-3 shape for the DL construction): coverage + monochrome
  cubes ⇒ first-match evaluation recovers gold everywhere (no ties).
-/
theorem decision_list_exact {nK nY : Nat}
    (rs : DecisionList nK nY) (G : Fin nK → Fin nY)
    (hC : Covers rs) (hM : Monochrome rs G) (k : Fin nK) :
    eval? rs k = some (G k) := by
  obtain ⟨w, hw, hmw⟩ := hC k
  exact eval?_eq_gold_of_mem_monochrome rs G k w hw hmw hM

/-- Under the soundness hypotheses, `eval?` never returns `none`. -/
theorem decision_list_eval_isSome {nK nY : Nat}
    (rs : DecisionList nK nY) (G : Fin nK → Fin nY)
    (hC : Covers rs) (hM : Monochrome rs G) (k : Fin nK) :
    (eval? rs k).isSome = true := by
  rw [decision_list_exact rs G hC hM k]; rfl

/-- Lift a product-of-literals cube to a flat `Cube` via decoding flat keys. -/
def cubeOfLits {m nV nK : Nat} (lits : Fin m → Lit nV)
    (decode : Fin nK → ProdKey m nV) : Cube nK where
  hit k := prodHit lits (decode k)

theorem cubeOfLits_hit {m nV nK : Nat} (lits : Fin m → Lit nV)
    (decode : Fin nK → ProdKey m nV) (k : Fin nK) :
    (cubeOfLits lits decode).hit k = prodHit lits (decode k) := rfl

end UnisaFormal.L1
