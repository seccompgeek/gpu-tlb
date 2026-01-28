# Blackwell GPU Support

## Overview

The extractor has been updated to support Blackwell GPUs (GB202 and later), which introduce a 5-level page table hierarchy with PD4 as the top level.

## Changes Made

### Architecture Support

- **Previous architectures (Hopper and earlier)**: 4-level page table (PD3 → PD2 → PD1 → PD0)
- **Blackwell architecture**: 5-level page table (PD4 → PD3 → PD2 → PD1 → PD0)

### Modified Files

1. **trans.h** - Already had PD4 enum value defined
2. **main.cpp** - Added `--blackwell` command-line option to enable PD4 mode
3. **page-map.cpp** - Updated to handle PD4 level:
   - Entry count: PD4 has 2 entries
   - Virtual address bit shift: 56 bits for PD4
   - Next level mapping: PD4 → PD3
4. **page-dir.cpp** - Adjusted indentation for proper output formatting
5. **page-tab.cpp** - Adjusted indentation for proper output formatting
6. **page.cpp** - Adjusted indentation for proper output formatting
7. **README.md** - Updated with usage instructions for Blackwell GPUs

### Page Table Hierarchy

#### Hopper and Earlier (4-level)
```
PD3 (4 entries, 47-bit shift)
└── PD2 (512 entries, 38-bit shift)
    └── PD1 (512 entries, 29-bit shift)
        └── PD0 (256 entries, 21-bit shift)
            ├── PT (Page Table for 4K/64K pages)
            └── 2MB Pages (HUGE)
```

#### Blackwell (5-level)
```
PD4 (2 entries, 56-bit shift) ← NEW LEVEL
└── PD3 (4 entries, 47-bit shift)
    └── PD2 (512 entries, 38-bit shift)
        └── PD1 (512 entries, 29-bit shift)
            └── PD0 (256 entries, 21-bit shift)
                ├── PT (Page Table for 4K/64K pages)
                └── 2MB Pages (HUGE)
```

## Usage

### For Hopper and Earlier
```bash
./extractor <dump_file>
```

### For Blackwell
```bash
./extractor <dump_file> --blackwell
```

## Example Output

### Blackwell Mode
```
Blackwell mode: Starting from PD4
PD4@0x000000005000
  0-->  PD3@0x000000006000
    0-->    PD2@0x000000007000
      0-->      PD1@0x000000008000
        9-->        PD0@0x00000000a000
          16-->          PT@0x00000000c000
            1-->4K-Page@0x000000020000 VA:0x0000000121001000
```

## Technical Details

### Virtual Address Breakdown (Blackwell)
- Bits [56]: PD4 index (2 entries = 1 bit)
- Bits [47-55]: PD3 index (4 entries = 2 bits, but only [47] used)
- Bits [38-46]: PD2 index (512 entries = 9 bits)
- Bits [29-37]: PD1 index (512 entries = 9 bits)
- Bits [21-28]: PD0 index (256 entries = 8 bits)
- Bits [12-20]: Page table index or page offset
- Bits [0-11]: Page offset (4KB pages)

### NV_MMU_VER3 Format
The Blackwell GPUs use the NV_MMU_VER3 format as defined in `dev_mmu.h`. The PD4 entries follow the same format as PD3, PD2, and PD1 entries:
- Bit [0]: IS_PTE (0 for PDE, 1 for PTE)
- Bits [2:1]: APERTURE (0=invalid, 1=video, 2=coherent, 3=non-coherent)
- Bits [5:3]: PCF (Page Cache Flags)
- Bits [51:12]: ADDRESS (physical address >> 12)

## Compatibility

The extractor maintains backward compatibility with Hopper and earlier architectures. When run without the `--blackwell` flag, it operates in the original PD3 mode.

## Related Files

For a standalone Blackwell implementation with more detailed parsing, see:
- `extractor_blackewll.cpp` - Alternative implementation with enhanced PTE flag parsing
