$ErrorActionPreference = 'Stop'
$src = 'G:\Qt\_src626\qt-everywhere-src-6.2.6'
$sdk = 'G:\Qt\6.2.4\msvc2019_64\include'

foreach ($f in @('configure.bat', 'qtbase\src\corelib\kernel\qcoreapplication.h', 'qtdeclarative\src\quick\qquickview.h', 'qtdeclarative\src\qml\qqmlcontext.h')) {
    $p = Join-Path $src $f
    $state = 'MISSING'
    if (Test-Path -LiteralPath $p) { $state = 'OK' }
    "{0,-58} {1}" -f $f, $state
}

# 我们实际用到的类 -> (SDK 头文件, 源码里的头文件相对路径)
$pairs = @(
    @('QtCore\qcoreapplication.h',   'qtbase\src\corelib\kernel\qcoreapplication.h'),
    @('QtWidgets\qapplication.h',    'qtbase\src\widgets\kernel\qapplication.h'),
    @('QtGui\qwindow.h',             'qtbase\src\gui\kernel\qwindow.h'),
    @('QtGui\qimage.h',              'qtbase\src\gui\image\qimage.h'),
    @('QtCore\qstring.h',            'qtbase\src\corelib\text\qstring.h'),
    @('QtCore\qurl.h',               'qtbase\src\corelib\io\qurl.h'),
    @('QtCore\qvariant.h',           'qtbase\src\corelib\kernel\qvariant.h'),
    @('QtCore\qdir.h',               'qtbase\src\corelib\io\qdir.h'),
    @('QtCore\qelapsedtimer.h',      'qtbase\src\corelib\kernel\qelapsedtimer.h'),
    @('QtQml\qqmlcontext.h',         'qtdeclarative\src\qml\qqmlcontext.h'),
    @('QtQml\qqmlengine.h',          'qtdeclarative\src\qml\qqmlengine.h'),
    @('QtQuick\qquickview.h',        'qtdeclarative\src\quick\items\qquickview.h'),
    @('QtQuick\qquickwindow.h',      'qtdeclarative\src\quick\items\qquickwindow.h'),
    @('QtCore\qobject.h',            'qtbase\src\corelib\kernel\qobject.h')
)

function Normalize([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { return $null }
    $lines = [IO.File]::ReadAllLines($path, (New-Object Text.UTF8Encoding $false))
    $out = New-Object System.Collections.Generic.List[string]
    foreach ($l in $lines) {
        $t = $l.Trim()
        if ($t -like '#include*') { continue }      # 只差 include 写法，忽略
        if ($t -eq '') { continue }
        $out.Add($t)
    }
    return , $out
}

"`n---- 6.2.4 SDK 头  vs  6.2.6 源码头（已忽略 #include 与空行）----"
"{0,-26} {1,8} {2,8} {3,8} {4}" -f '头文件', '624行', '626行', '差异行', '结论'
foreach ($pair in $pairs) {
    $a = Join-Path $sdk $pair[0]
    $b = Join-Path $src $pair[1]
    $na = Normalize $a
    $nb = Normalize $b
    if ($null -eq $na) { "{0,-26} SDK 侧缺失 {1}" -f $pair[0], $a; continue }
    if ($null -eq $nb) { "{0,-26} 源码侧缺失 {1}" -f $pair[0], $b; continue }

    $diff = Compare-Object -ReferenceObject $na -DifferenceObject $nb
    $d = ($diff | Measure-Object).Count
    $verdict = '一致'
    if ($d -gt 0) { $verdict = '有差异' }
    "{0,-26} {1,8} {2,8} {3,8}   {4}" -f (Split-Path $pair[0] -Leaf), $na.Count, $nb.Count, $d, $verdict

    if ($d -gt 0) {
        $diff | Select-Object -First 6 | ForEach-Object { "    [{0}] {1}" -f $_.SideIndicator, $_.InputObject.Trim() }
    }
}
