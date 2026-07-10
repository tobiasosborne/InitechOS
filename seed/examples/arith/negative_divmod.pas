{ negative_divmod.pas -- negative-operand Pascal div/mod (beads initech-tf3c;
  seed/codegen.c:44-50 flagged this "documented but not exercised by the
  seed gate"). Semantics (ISO 7185 / Turbo Pascal, both agree here): `div`
  truncates TOWARD ZERO; `mod` is defined by i = (i div j) * j + (i mod j),
  so `mod` takes the sign of the dividend (the LEFT operand), never the
  divisor. This is exactly what x86 `idiv` does natively after `cdq`
  sign-extends the dividend (seed/codegen.c OP_DIV/OP_MOD), so no
  sign-correction is needed in codegen for this seed's stack-machine emit.

  Golden provenance: HAND-COMPUTED from the definition above (not FPC-minted
  -- this seed's differential fixtures are hand-computed exact-serial goldens
  per the sibling arith/*.pas files, not ported from an external compiler):
    A: -17 div  5 = -3   (trunc(-3.4) = -3)
    B: -17 mod  5 = -2   (-17 = 5*(-3)  + (-2); sign matches dividend -17)
    C:  17 div -5 = -3   (trunc(-3.4) = -3)
    D:  17 mod -5 =  2   ( 17 = (-5)*(-3) +  2; sign matches dividend  17)
    E: -17 div -5 =  3   (trunc( 3.4) =  3)
    F: -17 mod -5 = -2   (-17 = (-5)*3   + (-2); sign matches dividend -17)
  Expect exact serial: "A=-3 B=-2 C=-3 D=2 E=3 F=-2"

  Verified by direct run under the QEMU oracle harness (Law 2): GREEN as-is,
  no codegen change required -- this fixture closes a test-coverage gap, not
  a bug (see the updated comment in seed/codegen.c). }
program NegativeDivMod;
begin
  write('A='); write(-17 div 5); write(' ');
  write('B='); write(-17 mod 5); write(' ');
  write('C='); write(17 div -5); write(' ');
  write('D='); write(17 mod -5); write(' ');
  write('E='); write(-17 div -5); write(' ');
  write('F='); writeln(-17 mod -5)
end.
