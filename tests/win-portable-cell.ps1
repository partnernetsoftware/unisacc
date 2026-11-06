param(
  [Parameter(Mandatory=$true)][string]$Exe,
  [Parameter(Mandatory=$true)][string]$Tgt,
  [Parameter(Mandatory=$true)][string]$Root
)
$ErrorActionPreference = 'Stop'
$posix = 'POSIX 部分 NOT_RUN: 缺 fork,waitpid,kill,F_SETFD,environ,F_SETFL'
function Invoke-Cwd([string]$file, [string]$cwd, [string[]]$arguments, [int]$ms, [string]$stdinPath, [string]$stdoutPath) {
  $info = [Diagnostics.ProcessStartInfo]::new()
  $info.FileName = $file
  $info.UseShellExecute = $false
  $info.WorkingDirectory = $cwd
  $info.RedirectStandardOutput = $true
  $info.RedirectStandardError = $true
  if ($stdinPath) { $info.RedirectStandardInput = $true }
  foreach ($item in $arguments) { $info.ArgumentList.Add($item) }
  $p = [Diagnostics.Process]::new(); $p.StartInfo = $info
  try {
    [void]$p.Start()
    if ($stdinPath) {
      $fs = [IO.File]::OpenRead($stdinPath)
      try { $fs.CopyTo($p.StandardInput.BaseStream); $p.StandardInput.Close() } finally { $fs.Dispose() }
    }
    $so = $p.StandardOutput.BaseStream
    if ($stdoutPath) {
      $fo = [IO.File]::Create($stdoutPath)
      try { $so.CopyTo($fo) } finally { $fo.Dispose() }
    } else { [void]$p.StandardOutput.ReadToEnd() }
    $se = $p.StandardError.ReadToEndAsync()
    if (!$p.WaitForExit($ms)) { $p.Kill($true); return @{ Code = 124; Err = 'timeout' } }
    return @{ Code = $p.ExitCode; Err = $se.Result }
  } catch { return @{ Code = -1; Err = "$_" } } finally { $p.Dispose() }
}
function Stage([string]$cwd, [string]$stage, [string[]]$stargs, [string]$inFile, [string]$outFile) {
  $a = @('stream', $stage) + $stargs
  return Invoke-Cwd $Exe $cwd $a 20000 (Join-Path $cwd $inFile) (Join-Path $cwd $outFile)
}
$stdioNote = "portable subset(7/9 子检查),POSIX 部分 NOT_RUN: 缺 fork,waitpid,kill,F_SETFD,environ; split portable=9 posix=0 (win 未跑 devfull 2 条)"
$streamNote = "portable subset(6/9 子检查),$posix; split portable=6 posix=3 (A,F-empty,G)"
# --- stdio ---
$scu = Join-Path $Root ('out/scu-' + ($Tgt -replace '/','-'))
New-Item -ItemType Directory -Force -Path $scu | Out-Null
Copy-Item (Join-Path $Root 'tests/cases/stdio-crossunit/a.c') $scu
Copy-Item (Join-Path $Root 'tests/cases/stdio-crossunit/b.c') $scu
Copy-Item (Join-Path $Root 'tests/cases/stdio-crossunit/main.c') $scu
$c = Invoke-Cwd $Exe $scu @('-o','probe.exe','a.c','b.c','main.c') 55000 $null $null
$stdioOk = $c.Code -eq 0
if ($stdioOk) {
  foreach ($mode in @('0','1','2')) {
    $want = $(if ($mode -ceq '2') { 'ABC' } else { 'AB' })
    $rp = Join-Path $scu 'result'; if (Test-Path $rp) { Remove-Item $rp }
    $r = Invoke-Cwd (Join-Path $scu 'probe.exe') $scu @($mode) 15000 $null $null
    if ($r.Code -ne 0) { $stdioOk = $false; break }
    if ([IO.File]::ReadAllText((Join-Path $scu 'result')) -cne $want) { $stdioOk = $false; break }
  }
}
Write-Output ("STDIO`t" + $(if ($stdioOk) { 'PASS' } else { 'FAIL' }) + "`t" + $(if ($stdioOk) { 0 } else { 1 }) + "`t$stdioNote")
# --- stream B C D E H F ---
$sp = Join-Path $Root ('out/sp-' + ($Tgt -replace '/','-'))
New-Item -ItemType Directory -Force -Path $sp | Out-Null
Set-Content -LiteralPath (Join-Path $sp 'plain.c') -Value "int f(int x){return x*2;}`nint main(void){return f(21)-42;}`n" -Encoding ascii
Set-Content -LiteralPath (Join-Path $sp 'hello.c') -Value "#include <stdio.h>`nint main(void){printf(`"hi %d\n`",42);return 0;}`n" -Encoding ascii
Set-Content -LiteralPath (Join-Path $sp 'a.c') -Value "int g(int);`nint main(void){return g(2)-4;}`n" -Encoding ascii
Set-Content -LiteralPath (Join-Path $sp 'b.c') -Value "int g(int x){return x+x;}`n" -Encoding ascii
Set-Content -LiteralPath (Join-Path $sp 'bad.c') -Value "int main(void){return 1`n" -Encoding ascii
function Image-Pipe([string]$src, [string]$out, [string]$tag, [string]$routeTgt) {
  $run = @('--route', "$routeTgt/run/O0", '--res', 'run-forward', '--source', $src)
  $img = @('--route', "$routeTgt/image/O0", '--source', $src)
  if ((Stage $sp 'e2' $run $src "$tag.0").Code -ne 0) { return 1 }
  if ((Stage $sp 'e1' $run "$tag.0" "$tag.1").Code -ne 0) { return 1 }
  if ((Stage $sp 'e3' $run "$tag.1" "$tag.2").Code -ne 0) { return 1 }
  if ((Stage $sp 'sidecar' @() "$tag.2" "$tag.3").Code -ne 0) { return 1 }
  if ((Stage $sp 'prune' $img "$tag.3" "$tag.4").Code -ne 0) { return 1 }
  if ((Stage $sp 'lower' $img "$tag.4" "$tag.5").Code -ne 0) { return 1 }
  return (Stage $sp 'elf' ($img + @('--raw')) "$tag.5" $out).Code
}
function Same([string]$a, [string]$b) {
  $pa = Join-Path $sp $a; $pb = Join-Path $sp $b
  if (-not (Test-Path -LiteralPath $pa) -or -not (Test-Path -LiteralPath $pb)) { return $false }
  $ba = [IO.File]::ReadAllBytes($pa); $bb = [IO.File]::ReadAllBytes($pb)
  if ($ba.Length -eq 0 -or $ba.Length -ne $bb.Length) { return $false }
  for ($i = 0; $i -lt $ba.Length; $i++) { if ($ba[$i] -ne $bb[$i]) { return $false } }
  return $true
}
function Hex32([byte[]]$b) {
  if ($null -eq $b -or $b.Length -eq 0) { return '-' }
  $n = [Math]::Min(32, $b.Length); $s = ''
  for ($i = 0; $i -lt $n; $i++) { $s += ('{0:x2}' -f $b[$i]) }
  return $s
}
function Clip400([string]$s) {
  if ([string]::IsNullOrEmpty($s)) { return '-' }
  $t = $s.Replace("`r", '\r').Replace("`n", '\n')
  if ($t.Length -gt 400) { return $t.Substring(0, 400) }
  return $t
}
function CmdStats([string]$file, [string[]]$arguments) {
  $n = 1 + $arguments.Count
  $len = $file.Length
  foreach ($a in $arguments) { $len += 1 + $a.Length }
  return @{ argc = $n; cmdlen = $len }
}
function Dump-Pair([string]$name, [int]$rcA, [int]$rcB, [string]$fa, [string]$fb) {
  $pa = Join-Path $sp $fa; $pb = Join-Path $sp $fb
  $ha = $(if (Test-Path -LiteralPath $pa) { [IO.File]::ReadAllBytes($pa) } else { [byte[]]@() })
  $hb = $(if (Test-Path -LiteralPath $pb) { [IO.File]::ReadAllBytes($pb) } else { [byte[]]@() })
  $off = -1
  $lim = [Math]::Min($ha.Length, $hb.Length)
  for ($i = 0; $i -lt $lim; $i++) { if ($ha[$i] -ne $hb[$i]) { $off = $i; break } }
  if ($off -lt 0 -and $ha.Length -ne $hb.Length) { $off = $lim }
  Write-Output ("SP_FAIL`t$name`trcA=$rcA`trcB=$rcB`tlenA=$($ha.Length)`tlenB=$($hb.Length)`tmismatch_off=$off`thexA=$(Hex32 $ha)`thexB=$(Hex32 $hb)`texe=$Exe`ttgt=$Tgt")
}
$npass = 0
Write-Output ("SP_ENTRY`texe=$Exe`ttgt=$Tgt")
$ref = Invoke-Cwd $Exe $sp @('-o','plain.ref','plain.c') 55000 $null $null
$pip = Image-Pipe 'plain.c' 'plain.pipe' 'p' $Tgt
$bok = ($ref.Code -eq 0 -and $pip -eq 0 -and (Same 'plain.ref' 'plain.pipe'))
if ($bok) { $npass++ } else { Dump-Pair 'B plain-image-pipe-cmp' $ref.Code $pip 'plain.ref' 'plain.pipe' }
$ref = Invoke-Cwd $Exe $sp @('-o','hello.ref','hello.c') 55000 $null $null
$pip = Image-Pipe 'hello.c' 'hello.pipe' 'h' $Tgt
$cok = ($ref.Code -eq 0 -and $pip -eq 0 -and (Same 'hello.ref' 'hello.pipe'))
if ($cok) { $npass++ } else { Dump-Pair 'C hello-stdio-image-pipe-cmp' $ref.Code $pip 'hello.ref' 'hello.pipe' }
# D two-source
$ref = Invoke-Cwd $Exe $sp @('-o','ab.ref','a.c','b.c') 55000 $null $null
$uok = $true
foreach ($src in @('a.c','b.c')) {
  $j = $(if ($src -ceq 'a.c') { 0 } else { 1 })
  $ua = @('--route', "$Tgt/unit", '--unit', "$j", '--source', $src)
  if ((Stage $sp 'e2' $ua $src "$src.u2").Code -ne 0) { $uok = $false }
  elseif ((Stage $sp 'e1' $ua "$src.u2" "$src.u1").Code -ne 0) { $uok = $false }
}
if ($uok) {
  $fr = Invoke-Cwd $Exe $sp @('stream','frame','--in','a.c=a.c.u1','--in','b.c=b.c.u1') 20000 $null (Join-Path $sp 'ab.frame')
  if ($fr.Code -ne 0) { $uok = $false }
  else {
    $ma = @('--route', "$Tgt/multi/run/O0", '--res', 'run-forward', '--source', 'a.c')
    $mi = @('--route', "$Tgt/multi/image/O0", '--source', 'a.c')
    if ((Stage $sp 'units' $ma 'ab.frame' 'm.0').Code -ne 0) { $uok = $false }
    elseif ((Stage $sp 'e3' $ma 'm.0' 'm.1').Code -ne 0) { $uok = $false }
    elseif ((Stage $sp 'sidecar' @() 'm.1' 'm.2').Code -ne 0) { $uok = $false }
    elseif ((Stage $sp 'prune' $mi 'm.2' 'm.3').Code -ne 0) { $uok = $false }
    elseif ((Stage $sp 'lower' $mi 'm.3' 'm.4').Code -ne 0) { $uok = $false }
    elseif ((Stage $sp 'elf' ($mi + @('--raw')) 'm.4' 'ab.pipe').Code -ne 0) { $uok = $false }
  }
}
if ($ref.Code -eq 0 -and $uok -and (Same 'ab.ref' 'ab.pipe')) { $npass++ } else { Dump-Pair 'D two-source-image-pipe-cmp' $ref.Code $(if ($uok) { 0 } else { 1 }) 'ab.ref' 'ab.pipe' }
# E 65 sources tape
$list = @(); $names = @()
for ($i = 0; $i -lt 65; $i++) {
  $nm = $(if ($i -eq 0) { 'm0.c' } else { "u$i.c" })
  $names += $nm
  if ($i -eq 0) { Set-Content -LiteralPath (Join-Path $sp $nm) -Value "int f1(int);`nint main(void){return f1(1)-65;}`n" -Encoding ascii }
  elseif ($i -lt 64) { Set-Content -LiteralPath (Join-Path $sp $nm) -Value "int f$($i+1)(int);`nint f$i(int x){return f$($i+1)(x+1);}`n" -Encoding ascii }
  else { Set-Content -LiteralPath (Join-Path $sp $nm) -Value "int f$i(int x){return x;}`n" -Encoding ascii }
}
$eok = $true
$eStep = '-'
$ePipe = @{ Code = 0; Err = '' }
$refArgs = @('-S','-o','s65.ref') + $names
$ref = Invoke-Cwd $Exe $sp $refArgs 55000 $null $null
if ($ref.Code -ne 0) { $eStep = 'direct-S' }
for ($i = 0; $i -lt 65 -and $eok; $i++) {
  $src = $names[$i]
  $ua = @('--route', "$Tgt/unit", '--unit', "$i", '--source', $src)
  $st = Stage $sp 'e2' $ua $src "$src.u2"
  if ($st.Code -ne 0) { $eok = $false; $eStep = "e2:$src"; $ePipe = $st }
  else {
    $st = Stage $sp 'e1' $ua "$src.u2" "$src.u1"
    if ($st.Code -ne 0) { $eok = $false; $eStep = "e1:$src"; $ePipe = $st }
  }
}
if ($eok) {
  $inargs = @('stream','frame')
  for ($i = 0; $i -lt 65; $i++) { $inargs += @('--in', ($names[$i] + '=' + $names[$i] + '.u1')) }
  $fr = Invoke-Cwd $Exe $sp $inargs 55000 $null (Join-Path $sp 's65.frame')
  if ($fr.Code -ne 0) { $eok = $false; $eStep = 'frame'; $ePipe = $fr }
  else {
    $ta = @('--route', "$Tgt/multi/tape/O0", '--source', 'm0.c')
    $st = Stage $sp 'units' $ta 's65.frame' 't.0'
    if ($st.Code -ne 0) { $eok = $false; $eStep = 'units'; $ePipe = $st }
    else {
      $st = Stage $sp 'e3' $ta 't.0' 't.1'
      if ($st.Code -ne 0) { $eok = $false; $eStep = 'e3'; $ePipe = $st }
      else {
        $st = Stage $sp 'tapecheck' ($ta + @('--raw')) 't.1' 's65.pipe'
        if ($st.Code -ne 0) { $eok = $false; $eStep = 'tapecheck'; $ePipe = $st }
      }
    }
  }
}
$eok2 = ($ref.Code -eq 0 -and $eok -and (Same 's65.ref' 's65.pipe'))
if ($eok2) { $npass++ } else {
  Dump-Pair 'E 65-source-tape-pipe-cmp' $ref.Code $(if ($eok) { 0 } else { 1 }) 's65.ref' 's65.pipe'
  $rs = CmdStats $Exe $refArgs
  $fs = CmdStats $Exe $(if ($null -ne $inargs) { $inargs } else { @('stream','frame') })
  Write-Output ("SP_E`tstep=$eStep`tref_rc=$($ref.Code)`tref_argc=$($rs.argc)`tref_cmdlen=$($rs.cmdlen)`tref_err=$(Clip400 $ref.Err)`tpipe_rc=$($ePipe.Code)`tframe_argc=$($fs.argc)`tframe_cmdlen=$($fs.cmdlen)`tpipe_err=$(Clip400 $ePipe.Err)`tcwd=$sp")
}
# H wrong target
$wrong = $(if ($Tgt -ceq 'win/x86_64') { 'lnx/x86_64' } else { 'win/x86_64' })
$pip = Image-Pipe 'plain.c' 'plain.wrong.pipe' 'w' $wrong
if ($pip -ne 0 -or -not (Same 'plain.ref' 'plain.wrong.pipe')) { $npass++ } else { Dump-Pair 'H wrong-target-pipe-not-equal' 0 $pip 'plain.ref' 'plain.wrong.pipe' }
# F reject
$ref = Invoke-Cwd $Exe $sp @('-o','bad.ref','bad.c') 20000 $null $null
$pip = Image-Pipe 'bad.c' 'bad.pipe' 'b' $Tgt
if ($ref.Code -eq 1 -and $pip -eq 1) { $npass++ } else { Write-Output ("SP_FAIL`tF reject-stderr-and-rc`trcA=$($ref.Code)`trcB=$pip`texe=$Exe`ttgt=$Tgt") }
$streamOk = $npass -eq 6
Write-Output ("STREAM`t" + $(if ($streamOk) { 'PASS' } else { 'FAIL' }) + "`t" + $(if ($streamOk) { 0 } else { 1 }) + "`t$streamNote npass=$npass/6")
exit 0
