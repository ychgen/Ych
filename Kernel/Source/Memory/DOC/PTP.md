# PTE-Tracking Page (PTP) — Revision 3

A **PTP** has a fixed size equal to `KR_PAGE_SIZE` (**4096 bytes**), matching a physical memory page.

PTPs are used directly through the VMM’s direct map and are allocated from the PMM.

---

## Root-Level PTP Layout

| Byte Offset | Field | Description |
|------------|------|-------------|
| 0–3 | - | Reserved. |
| 4–7 | NXT | Page ID of the next PTP. Set to 00000000h if no next PTP is present. |
| 8–9 | LIM | Maximum number of VPP entries in all PTPs *(currently 340: (4096 - 16) / 12)*. |
| 10–11 | NUM | Number of valid VPP entries currently stored. |
| 12-13 | LLL | Linked-Link Length. Basically amount of PTPs in use, root inclusive. |
| 14 | FLG | Flags:<br>Bit 0: reserved<br>Bit 1: `LF` (Large Flag)<br>• No longer does anything. Reserved.<br>Bit 2: `RL` (Root Level)<br>• Must be set to `1` for Root-Level PTP.<br>Other bits: reserved |
| 15 | REV | Revision number |
| 16-4095 | VPPEA | Virtual—PTE Pair Entries Array |

---

## Non-Root-Level PTP Layout

| Byte Offset | Field | Description |
|------------|------|-------------|
| 0–3 | PRV | Page ID of the previous PTP. |
| 4–7 | NXT | Page ID of the next PTP. |
| 8–9 | LORID | LOWORD of `Root-Level PTP`'s Page ID. |
| 10–11 | NUM | Number of valid VPP entries currently stored. |
| 12-13 | HIRID | HIWORD of `Root-Level PTP`'s Page ID. |
| 14 | FLG | Flags:<br>Bit 0: reserved<br>Bit 1: `LF` (Large Flag)<br>• No longer does anything. Reserved.<br>Bit 2: `RL` (Root Level)<br>• Must be set to `0` for Non-Root-Level PTP.<br>Other bits: reserved |
| 15 | REV | Revision number |
| 16-4095 | VPPEA | Virtual—PTE Pair Entries Array |

---

## VPP (Virtual—PTE Pair)

Each entry in `VPPEA` represents a mapping between a virtual address and a PTE.

| Byte Offset | Field | Description |
|------------|------|-------------|
| 0–7 | VAB | Base virtual address<br>• Always 4KiB-aligned (since 2MiB and 1GiB is also 4KiB-aligned) |
| 8–11 | PID | Page ID of the page that contains the paging structure itself. |

---
