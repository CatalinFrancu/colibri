### DAG Invariants

Let *u* and *v* be any nodes. Let *i* be an internal node (non-leaf).

#### Proof numbers and disproof numbers.

* *i*'s proof number is equal to its first child's disproof number (that is, the minimum disproof number among all children).
* *i*'s disproof number is the sum of its children's proof numbers (capped at `INFTY64`).
* *u*'s children are sorted in increasing order by disproof number, then in decreasing order by proof number.
* In the level 1 search, leaf (dis)proof numbers are 1/1.
* In the level 2 search, leaf (dis)proof numbers are copied over after a level 1 search.

#### Transposition tables and depths

* *u* has a depth defined as the length of the shortest path from the root to *u*. The root's depth is 0.
* Nodes are added to a hash map using Zobrist keys as hash keys.
* Each position can occur at most twice in the transposition table.
  * The first occurrence has a finite depth. This is called an *original* node.
  * The first occurrence, if present, has infinite depth and (dis)proof numbers ∞/∞ (drawn). The clone is used to prevent loops in the DAG.
* If *u* has an original node *v* among its children, then depth(*v*) = 1 + depth(*u*).
  * In particular, if *u* is expanded and one of its children, *v*, is already a known position with depth(*v) ≤ depth(*u*), then *u* is instead linked to *v*'s clone.
  * This ensures that there are no back edges in the DAG.
