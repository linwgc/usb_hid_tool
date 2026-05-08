# USB HID RACE Tool (Qt6)

## Goal
PC tool for `speaker_ref_design` to send/receive RACE CMD over USB HID.

## Features
- Qt6 GUI with editable `VID/PID` and HID report IDs.
- Normal mode:
  - Send single RACE command.
  - Send multiple commands in sequence (one command per line).
- Serial-number mode:
  - Manual write: click to write current serial number.
  - Auto write: loop detect USB device, write once when new unit is plugged.
  - Auto-increment serial number.
  - Optional serial parameters (Plant/Manufacturer/Product/Month/Year), only sequence number auto-increments.
  - Production flow: read MAC -> write serial -> append MAC/SN record to CSV.
  - Write-success check: validate RACE response `0x5B`, command ID match, status `0x00`.
  - Duplicate protection: block write when same MAC or serial exists in historical successful records.
  - Optional label print after write success (TCP printer, default ZPL over `9100`).
  - Build RACE command from template and write to DUT.
  - Reserved extension for barcode-printer integration.

## HID Packet Format
- Byte 0: Report ID (`OUT: 0x06`, `IN: 0x07`)
- Byte 1: Valid length
- Byte 2: Target (`0x00 local`, `0x80 remote`)
- Byte 3~61: RACE payload

## Architecture
`src/core`
- `race_packet`: HID packet pack/unpack.
- `race_command`: hex text and byte conversion.

`src/transport`
- `hid_transport`: thin wrapper around `hidapi` (open/read/write/poll).

`src/service`
- `race_service`: business flow for send/receive/sequence.
- `serial_number_service`: SN formatting and template-based command generation.

`src/ui`
- `main_window`: Qt Widgets UI and interactions.

## Serial Template
Template supports placeholders:
- `{SERIAL_ASCII}`: serial bytes in ASCII-hex form (space separated).
- `{SERIAL_HEX}`: raw string insertion (for special flows).

Example:
`05 5A 0B 00 07 1C 00 {SERIAL_ASCII}`

## Build (Windows, qmake + Qt6 IDE)
1. Install Qt6 and hidapi.
2. Open `air_race_hid_tool.pro` in Qt Creator.
3. If you placed official hidapi source at `usb_tool/hidapi`, no extra setting is needed.
   - Expected files:
     - `usb_tool/hidapi/hidapi/hidapi.h`
     - `usb_tool/hidapi/windows/hid.c`
4. (Optional) For prebuilt library mode, configure `HIDAPI_ROOT` in qmake step:
   - `HIDAPI_ROOT=F:/3rdparty/hidapi`
5. Build and run in Qt Creator.

If `hidapi.dll` is not found at runtime, copy it beside the generated `.exe` or add it to `PATH`.
If using local source mode (`usb_tool/hidapi`), no `hidapi.dll` is required.

## If build cache causes strange errors
After changing `.pro`, Qt Creator may skip qmake. Do this once:
1. `Build` -> `Run qmake`
2. `Build` -> `Clean Project`
3. `Build` -> `Rebuild Project`

## Next Steps
- Add command profile management (JSON import/export).
- Add response checker (expect bytes + timeout + retry).
- Add printer adapter interface:
  - `IPrinterAdapter::printBarcode(serial)`
  - Zebra/TSC implementation via SDK or command language.

## Data and Trace
- Runtime output directory: `Data/` (beside executable).
- Logs: `Data/logs/tool_YYYYMMDD.log` (timestamped lines for trace/debug).
- CSV records: `Data/csv/mac_sn_YYYYMMDD_XXX.csv`.
- CSV auto-rotation by file size (5 MB per file), append-safe across app restart.
