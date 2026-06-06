param(
  [string]$Port = "COM7",
  [ValidateSet("IDLE", "PLAN", "CODE", "TEST", "DONE", "BLOCK", "STATE")]
  [string]$State = "STATE",
  [string]$Message = "",
  [int]$BaudRate = 115200,
  [int]$TimeoutMs = 6000
)

$serial = [System.IO.Ports.SerialPort]::new($Port, $BaudRate, "None", 8, "One")
$serial.NewLine = "`n"
$serial.ReadTimeout = $TimeoutMs
$serial.WriteTimeout = $TimeoutMs
$serial.DtrEnable = $false
$serial.RtsEnable = $false

try {
  $serial.Open()
  Start-Sleep -Milliseconds 2500
  $serial.DiscardInBuffer()

  if ($State -eq "STATE") {
    $command = "STATE"
  } else {
    $cleanMessage = ($Message -replace '[^\x20-\x7E]', '').Trim()
    $command = "PROGRESS $State $cleanMessage".Trim()
  }

  $serial.WriteLine($command)
  $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
  $response = $null
  while ([DateTime]::UtcNow -lt $deadline) {
    try {
      $line = $serial.ReadLine().Trim()
      if ($line -eq "OK" -or $line.StartsWith("ERR ") -or $line.StartsWith("{")) {
        $response = $line
        break
      }
    }
    catch [System.TimeoutException] {
      break
    }
  }

  if (-not $response) {
    throw "No valid response from $Port."
  }

  [pscustomobject]@{
    Port = $Port
    Command = $command
    Response = $response
  } | Format-List
}
finally {
  if ($serial.IsOpen) {
    $serial.Close()
  }
  $serial.Dispose()
}
