```math
W := [a\text{-}z0\text{-}9]^+
```

```math
U := \bigl((<W) + (>W) + (>>W)\bigr)^*
```

```math
C := W W^*
```

```math
R :=
\left(
  \left(
    C U (\vert + \&)
  \right)^*
  C U
  (\& + \varepsilon)
\right)
+ \varepsilon
```