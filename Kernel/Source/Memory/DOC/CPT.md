# Commit-Tracking Page (CTP) — Revision 2

**NOTE**: This system was a thought experiment and has been dropped and has never been implemented. It still exists because one day, it might become useful...  

A **CTP** has a fixed size equal to `KR_PAGE_SIZE` (**4096 bytes**), matching a physical memory page.

CTPs are used directly through the VMM’s direct map and are allocated from the PMM.

---

## CTP Layout

| Byte Offset | Field | Description |
|------------|------|-------------|
| 0–7 | PRV | Page ID of the previous CTP |
| 8–15 | NXT | Page ID of the next CTP |
| 16–17 | LIM | Maximum number of VPP entries in this CTP *(currently 254: (4096 - 32) / 16)* |
| 18–19 | NUM | Number of valid VPP entries currently stored |
| 20 | FLG | Flags:<br>Bit 0: reserved<br>Bit 1: `LF` (Large Flag)<br>• 0 = 4KiB pages per VPP<br>• 1 = 2MiB pages per VPP (512 contiguous pages)<br>Other bits: reserved |
| 21 | - | Reserved |
| 22 | REV | Revision number |
| 23–31 | - | Reserved |
| 32–4095 | VPPEA | Array of VPP entries |

---

## VPP (Virtual–Physical Pair)

Each entry in `VPPEA` represents a mapping between a virtual address and a physical page (or range).

| Byte Offset | Field | Description |
|------------|------|-------------|
| 0–7 | VAB | Base virtual address<br>• 4KiB-aligned if `FLG.LF = 0`<br>• 2MiB-aligned if `FLG.LF = 1` |
| 8–15 | PID | Base physical page ID<br>• If `LF = 0`: single 4KiB page<br>• If `LF = 1`: 512 contiguous physical pages starting from this ID |

---

## Summary

A CTP defines the mapping between a virtual memory region and its committed physical pages using structured VPP entries, supporting both standard (4KiB) and large (2MiB) page mappings depending on the `LF` flag.