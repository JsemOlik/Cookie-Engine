if(NOT WIN32)
  return()
endif()

if(NOT DEFINED COOKIE_EDITOR_TARGET OR COOKIE_EDITOR_TARGET STREQUAL "")
  return()
endif()

if(NOT EXISTS "${COOKIE_EDITOR_TARGET}")
  # Nothing to unlock yet on first build.
  return()
endif()

# Guarded unlock step: only terminate CookieEditor process if its executable path
# matches the exact target output path being linked.
set(_ps_script [=[
$ErrorActionPreference = 'SilentlyContinue'
$target = [System.IO.Path]::GetFullPath($args[0])
Get-CimInstance Win32_Process -Filter "Name = 'CookieEditor.exe'" | ForEach-Object {
  if ($_.ExecutablePath) {
    $exe = [System.IO.Path]::GetFullPath($_.ExecutablePath)
    if ($exe.Equals($target, [System.StringComparison]::OrdinalIgnoreCase)) {
      Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue
    }
  }
}
]=])

execute_process(
  COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "${_ps_script}" "${COOKIE_EDITOR_TARGET}"
  RESULT_VARIABLE _unlock_result
)

# Do not fail the build on unlock step issues; linker will still report a real lock if present.
unset(_unlock_result)
