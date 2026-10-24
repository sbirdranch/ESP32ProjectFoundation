$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    function Build-Run($name, $sources, $includes) {
        $arguments = @('-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', 'test/host/stubs', '-I', 'include')
        foreach ($directory in $includes) { $arguments += @('-I', $directory) }
        $executable = ".pio/$name.exe"
        & g++ @arguments @sources -o $executable
        if ($LASTEXITCODE -ne 0) { throw "$name compilation failed" }
        & "./$executable"
        if ($LASTEXITCODE -ne 0) { throw "$name failed" }
    }
    Build-Run 'window_controller_test' @('test/host/window_controller_test.cpp', 'lib/WindowController/src/WindowController.cpp') @('lib/WindowController/src')
    Build-Run 'window_mqtt_test' @('test/host/window_mqtt_test.cpp', 'lib/WindowMqtt/src/WindowMqtt.cpp', 'lib/WindowController/src/WindowController.cpp') @('lib/WindowController/src', 'lib/WindowMqtt/src', '.pio/libdeps/esp32-s3-uart/ArduinoJson/src')
    $ledSources = @('lib/LedSwitch/src/LedSwitch.cpp', 'lib/StartupIndicator/src/StartupIndicator.cpp')
    $ledIncludes = @('lib/LedSwitch/src', 'lib/StartupIndicator/src')
    Build-Run 'window_led_test' (@('test/host/window_led_test.cpp') + $ledSources) $ledIncludes
    Build-Run 'startup_indicator_test' (@('test/host/startup_indicator_test.cpp') + $ledSources) $ledIncludes
} finally { Pop-Location }
