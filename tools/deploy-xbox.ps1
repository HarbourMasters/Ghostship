[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $Console,
    [Parameter(Mandatory = $true)] [string] $Package,
    [string] $Certificate,
    [string[]] $Dependency,
    [int] $Port = 11443,
    [string] $User,
    [string] $Password,
    [switch] $Clean
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Package)) { throw "Package not found: $Package" }
$pkg = Get-Item $Package
$name = $pkg.Name

$curl = (Get-Command curl.exe -ErrorAction SilentlyContinue).Source
if (-not $curl) { throw "curl.exe not found; it ships with Windows 10 1803 and newer." }

$base = "https://${Console}:${Port}"
$auth = @()
if ($User) { $auth = @('-u', "${User}:${Password}") }

Write-Host "Console  : $base"
$info = & $curl -k -s -m 20 @auth "$base/api/os/info"
if (-not $info) { throw "No response from Device Portal at $base. Is the console on and in Developer Mode?" }
Write-Host "Device   : $info"

$headers = & $curl -k -s -D - -o $null -m 20 @auth "$base/api/os/info"
$token = ([regex]::Match($headers, 'Set-Cookie:\s*CSRF-Token=([^;\s]+)')).Groups[1].Value
if (-not $token) { throw "Device Portal did not issue a CSRF-Token; cannot perform writes." }
$csrf = @('-H', "X-CSRF-Token: $token", '-b', "CSRF-Token=$token")

if ($Clean) {
    $pkgs = & $curl -k -s -m 30 @auth "$base/api/app/packagemanager/packages"
    $identity = [IO.Path]::GetFileNameWithoutExtension($name) -replace '_.*$', ''
    $existing = [regex]::Matches($pkgs, '"PackageFullName" : "([^"]*' + [regex]::Escape($identity) + '[^"]*)"')
    foreach ($m in $existing) {
        $full = $m.Groups[1].Value
        Write-Host "Removing : $full  (LocalState will be deleted)"
        & $curl -k -s -m 120 @auth @csrf -X DELETE "$base/api/app/packagemanager/package?package=$([uri]::EscapeDataString($full))" | Out-Null
    }
}

$form = @('-F', "$name=@$($pkg.FullName)")
foreach ($dep in $Dependency) {
    if (-not (Test-Path $dep)) { throw "Dependency not found: $dep" }
    $d = Get-Item $dep
    $form += @('-F', "$($d.Name)=@$($d.FullName)")
    Write-Host "Dep      : $($d.Name)"
}
if ($Certificate) {
    if (-not (Test-Path $Certificate)) { throw "Certificate not found: $Certificate" }
    $cer = Get-Item $Certificate
    $form += @('-F', "$($cer.Name)=@$($cer.FullName)")
    Write-Host "Cert     : $($cer.Name)"
}

Write-Host "Uploading $name ($([math]::Round($pkg.Length / 1MB, 1)) MB)..."
$uri = "$base/api/app/packagemanager/package?package=$([uri]::EscapeDataString($name))"
$response = (& $curl -k -s -m 1800 -w "`nHTTP %{http_code}" @auth @csrf -X POST $uri @form) -join "`n"
Write-Host $response

if ($response -notmatch 'HTTP (200|202|204)') {
    throw "Upload failed. Device Portal said: $response"
}

Write-Host "Installing..."
$identity = [IO.Path]::GetFileNameWithoutExtension($name) -replace '_.*$', ''
$wantVersion = $null
try {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($pkg.FullName)
    $entry = $zip.Entries | Where-Object { $_.FullName -eq 'AppxManifest.xml' }
    if ($entry) {
        $reader = New-Object System.IO.StreamReader($entry.Open())
        $manifest = $reader.ReadToEnd()
        $reader.Close()
        $wantVersion = ([regex]::Match($manifest, '<Identity[^>]*Version="([^"]+)"')).Groups[1].Value
    }
    $zip.Dispose()
} catch {
    Write-Warning "Could not read the version out of the package: $_"
}
if ($wantVersion) { Write-Host "Expecting: ${identity}_$wantVersion" }

for ($i = 0; $i -lt 120; $i++) {
    Start-Sleep -Seconds 5

    $state = & $curl -k -s -m 20 @auth "$base/api/app/packagemanager/state"
    if ($state -match '"Success" : false') {
        throw "Install failed: $state"
    }

    $pkgs = & $curl -k -s -m 30 @auth "$base/api/app/packagemanager/packages"
    $pattern = if ($wantVersion) {
        '"PackageFullName" : "(' + [regex]::Escape($identity) + '_' + [regex]::Escape($wantVersion) + '[^"]*)"'
    } else {
        '"PackageFullName" : "([^"]*' + [regex]::Escape($identity) + '[^"]*)"'
    }
    $m = [regex]::Match($pkgs, $pattern)
    if ($m.Success) {
        $installed = $m.Groups[1].Value
        Write-Host "Install complete: $installed"

        & $curl -k -s -m 30 @auth @csrf -X POST -H "Content-Length: 0" `
            "$base/api/debug/dump/usermode/crashcontrol?packageFullName=$([uri]::EscapeDataString($installed))&crashDumpEnabled=true" | Out-Null
        Write-Host "Crash dumps : enabled"
        exit 0
    }
}
throw "Package '$identity' $wantVersion never appeared in the console's package list; the install did not complete."
