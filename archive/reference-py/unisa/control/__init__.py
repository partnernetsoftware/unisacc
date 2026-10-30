"""The SGD control arm: training, its nets and its weight format. [TR]

Not on the shipping path.  The shipped weights are CONSTRUCTED from the gold
tables (unisa/construct.py, `unisa build-weights`); this package is kept as
the evidence for the paper's negative results -- where SGD reaches 1.000 on
one table and where it does not.  `python3 -m unisa train` runs it; nothing
else should.  See ARCHITECTURE.md.
"""
