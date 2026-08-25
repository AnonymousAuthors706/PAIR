import sympy as sp

# ============================================================
# Symbolic parameters
# ============================================================

C_i, B_i = sp.symbols('C_i B_i', positive=True)
T_k, T_1 = sp.symbols('T_k T_1', positive=True)
C_k, C_1 = sp.symbols('C_k C_1', nonnegative=True)
budget_k = sp.symbols('budget_k', nonnegative=True)

# Measured PAIR enforcement latency (microseconds)
C_PAIR = sp.Integer(137)

# ============================================================
# Integer release-count variables
# ============================================================

# Baseline:
# n_k = ceil(R_baseline / T_k)
# n_1 = ceil(R_baseline / T_1)

n_k, n_1 = sp.symbols(
    'n_k n_1',
    integer=True,
    positive=True
)

# PAIR:
# n_p = ceil(R_PAIR / T_1)

n_p = sp.symbols(
    'n_p',
    integer=True,
    positive=True
)

# ============================================================
# Response-time equations
# ============================================================

R_baseline = (
    C_i
    + B_i
    + n_k * C_k
    + n_1 * C_1
)

R_pair = (
    C_i
    + B_i
    + budget_k
    + C_PAIR
    + n_p * C_1
)

print("R_baseline =")
sp.pprint(R_baseline)

print("\nR_PAIR =")
sp.pprint(R_pair)

# ============================================================
# Solve R_PAIR <= R_baseline for C_k
# ============================================================

difference = sp.expand(R_pair - R_baseline)

print("\nR_PAIR - R_baseline =")
sp.pprint(difference)

Ck_solution = sp.solve(
    sp.Eq(difference, 0),
    C_k
)

print("\nBoundary value of C_k:")
sp.pprint(sp.simplify(Ck_solution[0]))

# ============================================================
# Fixed-point / ceiling consistency conditions
# ============================================================

baseline_conditions = [
    (n_k - 1) * T_k < R_baseline,
    R_baseline <= n_k * T_k,

    (n_1 - 1) * T_1 < R_baseline,
    R_baseline <= n_1 * T_1,
]

pair_conditions = [
    (n_p - 1) * T_1 < R_pair,
    R_pair <= n_p * T_1,
]

print("\nBaseline ceiling conditions:")
for condition in baseline_conditions:
    sp.pprint(condition)

print("\nPAIR ceiling conditions:")
for condition in pair_conditions:
    sp.pprint(condition)

# ============================================================
# Explicit symbolic threshold
# ============================================================

Ck_threshold = sp.simplify(
    (
        budget_k
        + C_PAIR
        + (n_p - n_1) * C_1
    ) / n_k
)

print("\nSymbolic C_k threshold:")
sp.pprint(
    sp.Ge(C_k, Ck_threshold)
)