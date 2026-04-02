#!/usr/bin/env pwsh
# Build script for Advanced-idot compiler
# Automatically detects LLVM and builds with appropriate settings

param(
    [switch]$Release,
    [switch]$NoCodegen,
    [switch]$Test,
    [switch]$Clean,
    [string]$LLVMPath = ""
)

Write-Host "Advanced-idot Build Script" -ForegroundColor Cyan
Write-Host "=========================" -ForegroundColor Cyan
Write-Host ""

# Clean if requested
if ($Clean) {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow
    cargo clean
    Write-Host "Clean complete!" -ForegroundColor Green
    exit 0
}

# Check for LLVM if codegen is needed
if (-not $NoCodegen) {
    Write-Host "Checking for LLVM..." -ForegroundColor Yellow
    
    # Check if LLVM path is provided
    if ($LLVMPath) {
        $env:LLVM_SYS_180_PREFIX = $LLVMPath
        Write-Host "Using provided LLVM path: $LLVMPath" -ForegroundColor Green
    }
    # Check environment variable
    elseif ($env:LLVM_SYS_180_PREFIX) {
        Write-Host "Using LLVM_SYS_180_PREFIX: $env:LLVM_SYS_180_PREFIX" -ForegroundColor Green
    }
    # Search common locations
    else {
        $commonPaths = @(
            "C:\LLVM-18",
            "C:\Program Files\LLVM-18",
            "C:\LLVM",
            "C:\Program Files\LLVM"
        )
        
        $found = $false
        foreach ($path in $commonPaths) {
            if (Test-Path $path) {
                # Check if it's actually LLVM 18
                $llvmConfigPath = Join-Path $path "bin\llvm-config.exe"
                $clangPath = Join-Path $path "bin\clang.exe"
                
                if (Test-Path $clangPath) {
                    $version = & $clangPath --version 2>&1 | Select-String "version (\d+)" | ForEach-Object { $_.Matches.Groups[1].Value }
                    
                    if ($version -eq "18") {
                        $env:LLVM_SYS_180_PREFIX = $path
                        Write-Host "Found LLVM 18 at: $path" -ForegroundColor Green
                        $found = $true
                        break
                    }
                    else {
                        Write-Host "Found LLVM at $path but version is $version (need 18)" -ForegroundColor Yellow
                    }
                }
            }
        }
        
        if (-not $found) {
            Write-Host ""
            Write-Host "WARNING: LLVM 18 not found!" -ForegroundColor Red
            Write-Host ""
            Write-Host "You have two options:" -ForegroundColor Yellow
            Write-Host "1. Install LLVM 18 and run this script again with -LLVMPath parameter" -ForegroundColor Yellow
            Write-Host "   Download: https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8" -ForegroundColor Yellow
            Write-Host ""
            Write-Host "2. Build without codegen (lexer + parser only):" -ForegroundColor Yellow
            Write-Host "   .\build.ps1 -NoCodegen" -ForegroundColor Yellow
            Write-Host ""
            
            $response = Read-Host "Build without codegen? (Y/n)"
            if ($response -eq "" -or $response -eq "Y" -or $response -eq "y") {
                $NoCodegen = $true
            }
            else {
                Write-Host "Build cancelled. Please install LLVM 18 or use -NoCodegen flag." -ForegroundColor Red
                exit 1
            }
        }
    }
}

# Build arguments
$buildArgs = @()

if ($NoCodegen) {
    Write-Host ""
    Write-Host "Building WITHOUT codegen (--no-default-features)..." -ForegroundColor Yellow
    $buildArgs += "--no-default-features"
}
else {
    Write-Host ""
    Write-Host "Building WITH codegen (LLVM enabled)..." -ForegroundColor Yellow
}

if ($Release) {
    Write-Host "Building in RELEASE mode..." -ForegroundColor Yellow
    $buildArgs += "--release"
}
else {
    Write-Host "Building in DEBUG mode..." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Running: cargo build $($buildArgs -join ' ')" -ForegroundColor Cyan
Write-Host ""

# Build
$buildCommand = "cargo build $($buildArgs -join ' ')"
Invoke-Expression $buildCommand

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Build failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green

# Run tests if requested
if ($Test) {
    Write-Host ""
    Write-Host "Running tests..." -ForegroundColor Yellow
    
    if ($NoCodegen) {
        cargo test --no-default-features
    }
    else {
        cargo test
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Tests failed!" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    
    Write-Host "All tests passed!" -ForegroundColor Green
}

# Show build artifacts
Write-Host ""
Write-Host "Build artifacts:" -ForegroundColor Cyan

if ($Release) {
    $artifactPath = "target\release\idotc.exe"
}
else {
    $artifactPath = "target\debug\idotc.exe"
}

if (Test-Path $artifactPath) {
    $size = (Get-Item $artifactPath).Length
    $sizeKB = [math]::Round($size / 1KB, 2)
    $sizeMB = [math]::Round($size / 1MB, 2)
    
    Write-Host "  $artifactPath" -ForegroundColor Green
    
    if ($sizeMB -gt 1) {
        Write-Host "  Size: $sizeMB MB" -ForegroundColor Gray
    }
    else {
        Write-Host "  Size: $sizeKB KB" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "Usage examples:" -ForegroundColor Cyan
Write-Host "  .\$artifactPath build examples\simple.idot --emit-llvm" -ForegroundColor Gray
Write-Host "  .\$artifactPath build examples\demo.idot -O --emit-llvm" -ForegroundColor Gray
Write-Host ""
