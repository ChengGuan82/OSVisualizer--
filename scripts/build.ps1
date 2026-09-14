param(
    [string]$QtRoot = 'F:\tools\QT\6.9.0\mingw_64',
    [string]$CompilerRoot = 'F:\tools\QT\Tools\mingw1310_64',
    [string]$CMake = 'F:\tools\QT\Tools\CMake_64\bin\cmake.exe',
    [string]$Ninja = 'F:\tools\QT\Tools\Ninja\ninja.exe'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
foreach ($item in @("$QtRoot\bin\windeployqt.exe", "$CompilerRoot\bin\g++.exe", $CMake, $Ninja)) {
    if (-not (Test-Path -LiteralPath $item)) { throw "找不到构建工具：$item。请通过脚本参数指定本机安装位置。" }
}
$env:PATH = "$CompilerRoot\bin;$QtRoot\bin;" + $env:PATH
& $CMake -S $projectRoot -B "$projectRoot\build" -G Ninja "-DCMAKE_MAKE_PROGRAM=$Ninja" "-DCMAKE_CXX_COMPILER=$CompilerRoot/bin/g++.exe" "-DCMAKE_PREFIX_PATH=$QtRoot" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw '配置失败' }
& $CMake --build "$projectRoot\build" --parallel 4
if ($LASTEXITCODE -ne 0) { throw '编译失败' }
# The build directory needs Qt DLLs as well as the offscreen test plugin.
& "$QtRoot\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --compiler-runtime "$projectRoot\build\OSVisualizer.exe"
if ($LASTEXITCODE -ne 0) { throw '测试运行库部署失败' }
Copy-Item -LiteralPath "$QtRoot\plugins\platforms\qoffscreen.dll" -Destination "$projectRoot\build\platforms" -Force
& (Join-Path (Split-Path $CMake) 'ctest.exe') --test-dir "$projectRoot\build" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw '测试失败' }
$packageRoot = Join-Path $projectRoot 'Release\v2.0'
New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null
Copy-Item -LiteralPath "$projectRoot\build\OSVisualizer.exe" -Destination $packageRoot -Force
& "$QtRoot\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --compiler-runtime "$packageRoot\OSVisualizer.exe"
if ($LASTEXITCODE -ne 0) { throw 'Qt 运行库部署失败' }
Copy-Item -LiteralPath "$projectRoot\README.md" -Destination $packageRoot -Force
Copy-Item -LiteralPath "$projectRoot\docs" -Destination $packageRoot -Recurse -Force
Copy-Item -LiteralPath "$projectRoot\screenshots" -Destination $packageRoot -Recurse -Force
Copy-Item -LiteralPath "$QtRoot\plugins\platforms\qoffscreen.dll" -Destination "$packageRoot\platforms" -Force
Write-Host "构建、测试和打包完成：$packageRoot\OSVisualizer.exe"
