\[
W := [a\text{-}z0\text{-}9]^+
\]

\[
U := \bigl((<W) + (>W) + (>>W)\bigr)^*
\]

\[
C := W W^*
\]

\[
R :=
\left(
  \left(
    C U (\texttt{|} + \&) 
  \right)^*
  C U
  (\& + \varepsilon)
\right)
+ \varepsilon
\]