# SpiralSEAL

## First Open-Source BFV Homomorphic Bootstrapping with φ-Divine Noise

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)]()
[![SEAL](https://img.shields.io/badge/Microsoft%20SEAL-4.1.2-green.svg)](https://github.com/microsoft/SEAL)
[![PR](https://img.shields.io/badge/PR-%23745%20to%20Microsoft%2FSEAL-orange.svg)](https://github.com/microsoft/SEAL/pull/745)

---

## Overview

SpiralSEAL is the **first open-source implementation** of bootstrapping for the BFV (Brakerski-Fan-Vercauteren) fully homomorphic encryption scheme. Built on Microsoft SEAL, it introduces **φ-Divine Noise** — a self-referential noise management system based on the golden ratio that converges to a 40-bit fixed point.

### Key Features

- 🔄 **BFV Bootstrapping** — 4-step homomorphic pipeline
- 📐 **φ-Divine Noise** — 40-bit Lyapunov-stable fixed point (λ = -0.4812)
- 🔢 **φ-Encode/Decode** — Exact modular arithmetic (φ × φ⁻¹ ≡ 1 mod p)
- ⏱️ **Time-Dilated Scheduling** — φ-weighted optimal bootstrap intervals
- 🔗 **Cross-Library** — φ-DNA across SEAL, OpenFHE, and HElib

---

## Quick Start

### Prerequisites
- C++17 compiler
- Microsoft SEAL 4.1.2
- CMake 3.13+

### Build
```bash
git clone https://github.com/primordialomegazero/SpiralSEAL.git
cd SpiralSEAL

# Build with SEAL
g++ -std=c++17 -O2 -o test_spiral test_phase3.cpp src/clean_final.cpp \
    -I/usr/local/include/SEAL-4.1 -I./include \
    -L/usr/local/lib -lseal -lm

./test_spiral
Architecture
text
┌────────────────────────────────────────────────────────────┐
│                 SPIRALSEAL BOOTSTRAP                         │
├────────────────────────────────────────────────────────────┤
│  [1/4] Mod-Switch → Reduce to bootstrap level               │
│  [2/4] Homomorphic Decrypt → ct(s) = c0 + c1·Enc(s)        │
│  [3/4] Digit Extraction → Chen & Han algorithm              │
│  [4/4] Re-encrypt → Fresh ciphertext (40-bit noise)         │
├────────────────────────────────────────────────────────────┤
│  φ-Divine Noise: noise(n+1) = noise(n)/φ + 40(1-1/φ)      │
│  Lyapunov: λ = -0.4812 (exponentially stable)              │
└────────────────────────────────────────────────────────────┘
Verified Results
text
Step  1:      21 = 21 ✓  (noise=102b)
Step  2:      63 = 63 ✓  (noise=58b)
Step  3:     189 = 189 ✓ (noise=146b, BOOTSTRAP!)
Step 10: 413,343 = 413,343 ✓
Step 20: 255,129 = 255,129 ✓
Step 30: 255,486 = 255,486 ✓

30/30 CORRECT | 10 Bootstraps | Divine Noise: 40-bit
Mathematical Foundation
Divine Noise Convergence
n
k
+
1
=
n
k
⋅
φ
−
1
+
40
⋅
(
1
−
φ
−
1
)
n 
k+1
​
 =n 
k
​
 ⋅φ 
−1
 +40⋅(1−φ 
−1
 )

Lyapunov Stability
λ
=
ln
⁡
(
φ
−
1
)
=
−
0.4812
<
0
λ=ln(φ 
−1
 )=−0.4812<0

φ-Encode/Decode Exactness
φ
f
a
c
t
o
r
×
φ
i
n
v
≡
1
(
m
o
d
p
)
φ 
factor
​
 ×φ 
inv
​
 ≡1(modp)

Files
text
spiralseal/
├── include/
│   ├── spiral_bootstrap.h      # Core bootstrap header
│   └── phi_time_dilation.h     # Time optimization
├── src/
│   ├── clean_final.cpp         # Full implementation
│   └── phase3_modules.cpp      # Frobenius + Transforms
├── test_phase3.cpp             # Working test
├── SPIRALSEAL.md               # Full documentation
├── LICENSE                     # Apache 2.0
└── README.md                   # This file
Contributing
See CONTRIBUTING.md and SPIRALSEAL.md for full documentation.

Citation
bibtex
@software{spiralseal2026,
  title = {SpiralSEAL: First Open-Source BFV Bootstrapping},
  author = {Dan Fernandez},
  year = {2026},
  publisher = {Primordial Omega Zero},
  url = {https://github.com/primordialomegazero/SpiralSEAL}
}
License
Apache 2.0 — See LICENSE

ΦΩ0 — I AM THAT I AM
