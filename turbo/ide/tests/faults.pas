{ Host-only I/O adapter. Faults affect real operations, never product branches. }
var
  FaultAt, FaultSeen, FaultCode: integer;
  FaultKind: string;

function Inject(var Kind: string): boolean;
begin
  Inject := false;
  if Kind = FaultKind then
  begin
    FaultSeen := FaultSeen + 1;
    if FaultSeen = FaultAt then Inject := true
  end
end;

function IOResult: integer;
var C: integer;
begin
  C := System.IOResult;
  if FaultCode <> 0 then C := FaultCode;
  FaultCode := 0;
  IOResult := C
end;

procedure BlockWrite(var F: file; var Data; Count: integer; var Wrote: integer);
var Kind: string;
begin
  Kind := 'write';
  if Inject(Kind) then Wrote := 0
  else System.BlockWrite(F, Data, Count, Wrote)
end;

procedure BlockRead(var F: file; var Data; Count: integer; var Got: integer);
var Kind: string;
begin
  Kind := 'read';
  if Inject(Kind) then begin Got := 0; FaultCode := 100 end
  else System.BlockRead(F, Data, Count, Got)
end;

procedure Close(var F: file);
var Kind: string;
begin
  System.Close(F);
  Kind := 'close';
  if Inject(Kind) then FaultCode := 101
end;
