<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Copies the source tree excluding various .git and control files.

.DESCRIPTION
This script is used to extract a minimal source tree for testing.

.PARAMETER FilePath
Indicates the root of the tree to copy to.

.PARAMETER Overwrite
Indicates overwrite of existing content if present.

.PARAMETER Clean
Delete content in the target directory before copying.

.PARAMETER AssetsOnly
Only copy asset files, not source code.

.EXAMPLE
copysourcetree.ps1 -Destination D:\temp\abc

.EXAMPLE
Remove-Item D:\temp\abc -Recurse -force -ErrorAction SilentlyContinue | Out-Null
New-Item -Path D:\Temp -Name "abc" -ItemType Directory -ErrorAction SilentlyContinue | Out-Null
.\build\copysourcetree.ps1 -Destination D:\temp\abc
robocopy /mir D:\temp\abc \\durfs\durango\TestContent\samples\nightly_dist_directxtk

Update internal test share.
#>

param(
    [Parameter(Mandatory)]
    [string]$Destination,
    [switch]$Quiet,
    [switch]$Overwrite,
    [switch]$Clean,
    [switch]$AssetsOnly
)

$xcopyFlags = "/Y/S"
if($Quiet) {
    $xcopyFlags += "/Q"
}
else {
    $xcopyFlags += "/F"
}

function Copy-Source {

    param(
        [Parameter(Mandatory)]
        [string]$Path,
        [Parameter(Mandatory)]
        [string]$Destination
        )

    $filters = @("*.cpp",
        "*.h", "*.inl", "*.inc",
        "*.cmd",
        "*.hlsl", "*.hlsli", "*.fx", "*.fxh",
        "*.sln", "*.vcxproj", "*.vcxproj.filters",
        "*.config", "*.mgc", "*.appxmanifest", "*.manifest")

    $excludefile = Split-Path -Path $PSScriptRoot -Parent
    $excludefile = Join-Path $excludefile -Child "build"
    $excludefile = Join-Path $excludefile -Child "copysourcetree.flt"

    $filters | ForEach-Object {
        $files = Join-Path -Path $Path -ChildPath $_
        xcopy $xcopyFlags /EXCLUDE:$excludefile $files $Destination
        if ($LastExitCode -ne 0) {
            Write-Error "Failed copying source files" -ErrorAction Stop
        }
    }
}

function Copy-Asset {

    param(
        [Parameter(Mandatory)]
        [string]$Path,
        [Parameter(Mandatory)]
        [string]$Destination
        )

    $filters = @("*.spritefont", "*.bmp", "*.dds", "*.png", "*.jpg", "*.tif", "*.tiff",
        "*.sdkmesh*", "*.cmo", "*._obj", "*.mtl", "*.vbo",
        "*.wav", "*.xwb")

    $excludefile = Split-Path -Path $PSScriptRoot -Parent
    $excludefile = Join-Path $excludefile -Child "build"
    $excludefile = Join-Path $excludefile -Child "copysourcetree.flt"

    $filters | ForEach-Object {
        $files = Join-Path -Path $Path -ChildPath $_
        xcopy $xcopyFlags /EXCLUDE:$excludefile $files $Destination
        if ($LastExitCode -ne 0) {
            Write-Error "Failed copying asset files" -ErrorAction Stop
        }
    }
}

if (-Not (Test-Path $Destination)) {
    Write-Error "ERROR: -Destination folder does not exist" -ErrorAction Stop
}

$destdir = Join-Path $Destination -ChildPath "DirectXTK12"

$targetreadme = Join-Path -Path $destdir -ChildPath "README.md"

if ((Test-Path $targetreadme) -And (-Not $Overwrite)) {
    Write-Error "ERROR: Destination folder contains files. Use -Overwrite to proceed anyhow." -ErrorAction Stop
}

if($Clean) {
    Write-Host "Clean..."
    Remove-Item $destdir -Recurse -force -ErrorAction SilentlyContinue | Out-Null
}

New-Item -Path $Destination -Name "DirectXTK12" -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

$sourcedir = Split-Path -Path $PSScriptRoot -Parent

$readme = Join-Path -Path $sourcedir -ChildPath "README.md"
$license = Join-Path -Path $sourcedir -ChildPath "LICENSE"

Copy-Item $readme -Destination $destdir
Copy-Item $license -Destination $destdir

if (-Not $AssetsOnly)
{
    Copy-Source -Path $sourcedir -Destination $destdir
}

Copy-Asset -Path $sourcedir -Destination $destdir
