Using Namespace System
$url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-14.0.6/LLVM-14.0.6-win64.exe"
$llvmInstallerPath = Join-Path $PSScriptRoot "LLVM-14.0.6-win64.exe"
$clangFormatFilePath = Join-Path $PSScriptRoot "clang-format.exe"
$requiredVersion = "clang-format version 14.0.6"
$currentVersion = ""

function Test-7ZipInstalled {
    $sevenZipPath = "C:\Program Files\7-Zip\7z.exe"
    return Test-Path $sevenZipPath -PathType Leaf
}

if (Test-Path $clangFormatFilePath) {
    $currentVersion = & $clangFormatFilePath --version
    if (-not ($currentVersion -eq $requiredVersion)) {
        Remove-Item $clangFormatFilePath -Force
    }
}

if (-not (Test-Path $clangFormatFilePath) -or ($currentVersion -ne $requiredVersion)) {
    if (-not (Test-7ZipInstalled)) {
        Write-Host "7-Zip is not installed. Please install 7-Zip and run the script again."
        exit
    }

    $wc = New-Object net.webclient
    $wc.Downloadfile($url, $PSScriptRoot + $llvmInstallerPath)

    $sevenZipPath = "C:\Program Files\7-Zip\7z.exe"
    $specificFileInArchive = "bin\clang-format.exe"
    & "$sevenZipPath" e $llvmInstallerPath $specificFileInArchive

    Remove-Item $llvmInstallerPath -Force
}

$clangFormat = Join-Path $PSScriptRoot "clang-format.exe"
$basePath = Join-Path $PSScriptRoot "src"
$files = @(Get-ChildItem -Path $basePath -Recurse -File `
    | Where-Object { $_ -ne $null -and ($_.Extension -eq '.c' -or $_.Extension -eq '.cpp' -or `
                     $_.Extension -eq '.h' -or $_.Extension -eq '.hpp') })

for ($i = 0; $i -lt $files.Length; $i++) {
    $file = $files[$i]
    if ($file -eq $null -or $file.FullName -eq $null) { continue }
    $relativePath = $file.FullName.Substring($basePath.Length + 1)
    Write-Host "Formatting [$($i+1)/$($files.Length)] $relativePath"
    & $clangFormat -i $file.FullName
}
