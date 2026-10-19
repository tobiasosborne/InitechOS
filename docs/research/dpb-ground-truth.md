# DPB ground truth -- INT 21h AH=1Fh / AH=32h and the DOS 3.x Drive Parameter Block

Lane: dpb (WL-0167, bead initech-dpb1). Acquired 2026-10-17 because the
local IBM DOS 3.30 Technical Reference does NOT document the DPB: its
function listing (80X0945, pp6-34..6-35) names both slots only as
`1FH Reserved by DOS` and `32H Reserved by DOS`, and the function pages
run from 1Ch (p6-84) straight to 21h (p6-85). The layout below is
therefore taken from the three acquired period sources in Sec 1, which
agree field-for-field. Nothing here is cited from memory.

## 1. Provenance

| Source | Retrieved from | SHA-256 | License |
|---|---|---|---|
| MS-DOS 2.0 `DOSSYM.ASM` (dpb STRUC, function numbers) | https://raw.githubusercontent.com/microsoft/MS-DOS/main/v2.0/source/DOSSYM.ASM (repo commit 2d04cacc5322951f187bb17e017c12920ac8ebe2) | `6ced8dec5ea565c9ae782051735550bd9fbae69b983e645425d47075aa8b1dc3` | MIT (Microsoft, 2014/2018 release) |
| MS-DOS 2.0 `GETSET.ASM` ($GET_DEFAULT_DPB / $GET_DPB / $SETDPB) | .../v2.0/source/GETSET.ASM | `b4e3b8cf88ac743aa51037ad8366a35d3bc0bb909f74ce13b8c9d754c4a74f00` | MIT |
| MS-DOS 2.0 `MSINIT.ASM` (DPB chain, end marker) | .../v2.0/source/MSINIT.ASM | `83f8ae8f9d4aaa5eeda6ed9bdb52bb72f62758a1428bf92a84dfc2e4b63a7e56` | MIT |
| MS-DOS 4.0 `INC/DPB.INC` (SCCSID `dpb.asm 1.1 85/04/10`) | .../v4.0/src/INC/DPB.INC | `7c64d7b974db6d2ae540b05d0ff57eaabdf1bdc7c77165eb6a2796de28290ccb` | MIT (Microsoft, 2024 release) |
| MS-DOS 4.0 `DOS/MISC.ASM` ($GET_DPB, $SETDPB with AC000/AN000 change marks) | .../v4.0/src/DOS/MISC.ASM | `ac7c9d5590d4e6473bc2cfe495c7e6441f7f6ff1a4c44bb8c6b94cf509b68042` | MIT |
| Ralf Brown's Interrupt List, release 61, `INTERRUP.F` (AH=1Fh, AH=32h, Table 01395) | https://www.cs.cmu.edu/~ralf/interrupt-list/inter61b.zip (zip `713e9e218eda694ed2bac8d5f02378f935f3e3132b79c92c44c14101e76b88a7`) | `58be5f66a47ba9c1c7b25ca2a6a702c5efc5599f11101bf8ac2efd42b701e4d8` | freely redistributable list |
| IBM DOS 3.30 Technical Reference 80X0945 (local, initech-manuals-ref) | bitsavers, see initech-manuals-ref/SOURCES.md | `e986bd10b6e90bdbc97dde3d5f8adfd2bd331ddf1ff962b31285375c7cd270b4` | -- |

Why these bracket DOS 3.3: the 2.0 source has the byte `dpb_FAT_size` and the
2.x current-directory tail; the 4.0 source marks every 4.0 change with
`;AC000;` (changed) / `;AN000;` (new). In 4.0's `$SETDPB` the `dpb_FAT_size`
store is `STOSW ;AC000;;>32mb` -- i.e. the pre-4.0 (3.x) code was the 2.0
`STOSB` -- while the `dpb_next_free` / `dpb_free_cnt` initialisation carries
no 4.0 mark, so it predates 4.0. RBIL states the same 3.x layout explicitly.

## 2. Verbatim excerpts (tabs expanded, CR stripped)

### 2.1 MS-DOS 2.0 DOSSYM.ASM

```
dpb     STRUC
dpb_drive           DB      ?   ; Logical drive # assoc with DPB (A=0,B=1,...)
dpb_UNIT            DB      ?   ; Driver unit number of DPB
dpb_sector_size     DW      ?   ; Size of physical sector in bytes
dpb_cluster_mask    DB      ?   ; Sectors/cluster - 1
dpb_cluster_shift   DB      ?   ; Log2 of sectors/cluster
dpb_first_FAT       DW      ?   ; Starting record of FATs
dpb_FAT_count       DB      ?   ; Number of FATs for this drive
dpb_root_entries    DW      ?   ; Number of directory entries
dpb_first_sector    DW      ?   ; First sector of first cluster
dpb_max_cluster     DW      ?   ; Number of clusters on drive + 1
dpb_FAT_size        DB      ?   ; Number of records occupied by FAT
dpb_dir_sector      DW      ?   ; Starting record of directory
dpb_driver_addr     DD      ?   ; Pointer to driver
dpb_media           DB      ?   ; Media byte
dpb_first_access    DB      ?   ; This is initialized to -1 to force a media
                                ; check the first time this DPB is used
dpb_next_dpb        DD      ?   ; Pointer to next Drive parameter block
dpb_current_dir     DW      ?   ; Cluster number of start of current directory
                                ; 0 indicates root, -1 indicates invalid
                                ; (disk ? changed)
dpb_dir_text        DB      DIRSTRLEN DUP(?)
                                ; ASCIZ string of current directory
dpb     ENDS

DPBSIZ  EQU     SIZE dpb        ; Size of the structure in bytes

DSKSIZ  =       dpb_max_cluster ; Size of disk (temp used during init only)
GET_DEFAULT_DRIVE               EQU 25  ; 25     19
GET_DEFAULT_DPB                 EQU 31  ; 31     1F
GET_DPB                         EQU 50  ; 50     32
```

### 2.2 MS-DOS 2.0 GETSET.ASM -- the call contract

```
        procedure   $GET_DEFAULT_DPB,NEAR
ASSUME  DS:NOTHING,ES:NOTHING

; Inputs:
;       DL = Drive number (always default drive for call 31)
; Function:
;       Return pointer to drive parameter table for default drive
; Returns:
;       DS:BX points to the DPB
;       AL = 0 If OK, = -1 if bad drive (call 50 only)

        MOV     DL,0
        entry   $GET_DPB
        PUSH    SS
        POP     DS
ASSUME  DS:DOSGROUP
        MOV     AL,DL
        invoke  GETTHISDRV
        JC      ISNODRV
        invoke  FATREAD
        invoke  get_user_stack
ASSUME  DS:NOTHING
        MOV     [SI.user_BX],BP
        MOV     [SI.user_DS],ES
        XOR     AL,AL
        return

ISNODRV:
        MOV     AL,-1
        return
$GET_Default_dpb    ENDP
```

### 2.3 MS-DOS 2.0 MSINIT.ASM -- chain links, end of list, first-access flag

```
        MOV     CL,[NUMIO]      ; Number of DPBs
        XOR     CH,CH
SETFINDPB:
        MOV     WORD PTR ES:[SI.dpb_next_dpb+2],ES
        MOV     ES:[SI.dpb_first_access],-1      ; Never accessed before
        ADD     SI,DPBSIZ       ; Point to next DPB
        LOOP    SETFINDPB
        SUB     SI,DPBSIZ
        MOV     WORD PTR ES:[SI.dpb_next_dpb+2],-1
        MOV     DI,OFFSET DOSGROUP:MEMSTRT    ; Current start of DPBs
        ADD     DI,dpb_next_dpb      ; Point at dpb_next_dpb field
        MOV     CL,[NUMIO]
        XOR     CH,CH
TRUEDPBAD:
        ADD     AX,DPBSIZ       ; Compute address of next DPB
        STOSW                   ; Set the link to next DPB
        ADD     DI,DPBSIZ-2     ; Point at next address
        LOOP    TRUEDPBAD
        SUB     DI,DPBSIZ       ; Point at last dpb_next_dpb field
        MOV     AX,-1
        STOSW                   ; End of list
```

### 2.4 MS-DOS 4.0 DPB.INC

```
;       SCCSID = @(#)dpb.asm    1.1 85/04/10
;       SCCSID = @(#)dpb.asm    1.1 85/04/10
dpb     STRUC
dpb_drive           DB      ?           ; Logical drive # assoc with DPB (A=0,B=1,...)
dpb_UNIT            DB      ?           ; Driver unit number of DPB
dpb_sector_size     DW      ?           ; Size of physical sector in bytes
dpb_cluster_mask    DB      ?           ; Sectors/cluster - 1
dpb_cluster_shift   DB      ?           ; Log2 of sectors/cluster
dpb_first_FAT       DW      ?           ; Starting record of FATs
dpb_FAT_count       DB      ?           ; Number of FATs for this drive
dpb_root_entries    DW      ?           ; Number of directory entries
dpb_first_sector    DW      ?           ; First sector of first cluster
dpb_max_cluster     DW      ?           ; Number of clusters on drive + 1
dpb_FAT_size        DW      ?           ;;Number of records occupied by FAT
dpb_dir_sector      DW      ?           ; Starting record of directory
dpb_driver_addr     DD      ?           ; Pointer to driver
dpb_media           DB      ?           ; Media byte
dpb_first_access    DB      ?           ; This is initialized to -1 to force a media
                                        ; check the first time this DPB is used
dpb_next_dpb        DD      ?           ; Pointer to next Drive parameter block
dpb_next_free       DW      ?           ; Cluster # of last allocated cluster
dpb_free_cnt        DW      ?           ; Count of free clusters, -1 if unknown
dpb     ENDS
```

### 2.5 MS-DOS 4.0 MISC.ASM -- $GET_DPB and $SETDPB (BPB -> DPB arithmetic)

```
        procedure   $GET_DEFAULT_DPB,NEAR
ASSUME  DS:NOTHING,ES:NOTHING

        MOV     DL,0

        entry   $GET_DPB
; Same as above only drive passed in DL (0=default, 1=A, 2=B, ...)

        context DS
        MOV     AL,DL
        invoke  GETTHISDRV              ; Get CDS structure
        JC      ISNODRV                 ; no valid drive
        LES     DI,[THISCDS]            ; check for net CDS
        TEST    ES:[DI.curdir_flags],curdir_isnet
        JNZ     ISNODRV                 ; No DPB to point at on NET stuff
        EnterCrit CritDisk
        invoke  FATRead_CDS             ; Force Media Check and return DPB
        LeaveCrit CritDisk
        JC      ISNODRV                 ; User FAILed to I 24, only error we
                                        ;   have.
        invoke  get_user_stack
ASSUME  DS:NOTHING
        MOV     [SI.user_BX],BP
        MOV     [SI.user_DS],ES
        XOR     AL,AL
        return

ISNODRV:
        MOV     AL,-1
        return
EndProc $GET_Default_dpb

        procedure   $SETDPB,NEAR
ASSUME  DS:NOTHING,ES:NOTHING

; Inputs:
;       ES:BP Points to DPB
;       DS:SI Points to BPB
; Function:
;       Build a correct DPB from the BPB
; Outputs:
; ES:BP and DS preserved all others destroyed

        MOV     DI,BP
        ADD     DI,2                    ; Skip over dpb_drive and dpb_UNIT
        LODSW
        STOSW                           ; dpb_sector_size
        CMP     BYTE PTR [SI.BPFTCNT-2],0     ; FAT file system drive           ;AN000;
        JNZ     yesfat                        ; yes                             ;AN000;
        MOV     BYTE PTR ES:[DI.dpb_FAT_count-4],0
        JMP     setend                        ; NO                              ;AN000;
yesfat:
        MOV     DX,AX
        LODSB
        DEC     AL
        STOSB                           ; dpb_cluster_mask
        INC     AL
        XOR     AH,AH
LOG2LOOP:
        TEST    AL,1
        JNZ     SAVLOG
        INC     AH
        SHR     AL,1
        JMP     SHORT LOG2LOOP
SAVLOG:
        MOV     AL,AH
        STOSB                           ; dpb_cluster_shift
        MOV     BL,AL
        MOVSW                           ; dpb_first_FAT Start of FAT (# of reserved sectors)
        LODSB
        STOSB                           ; dpb_FAT_count Number of FATs
;       OR      AL,AL                   ; NONFAT ?                              ;AN000;
;       JZ      setend                  ; yes, don't do anything                ;AN000;
        MOV     BH,AL
        LODSW
        STOSW                           ; dpb_root_entries Number of directory entries
        MOV     CL,5
        SHR     DX,CL                   ; Directory entries per sector
        DEC     AX
        ADD     AX,DX                   ; Cause Round Up
        MOV     CX,DX
        XOR     DX,DX
        DIV     CX
        MOV     CX,AX                   ; Number of directory sectors
        INC     DI
        INC     DI                      ; Skip dpb_first_sector
        MOVSW                           ; Total number of sectors in DSKSIZ (temp as dpb_max_cluster)
        LODSB
        MOV     ES:[BP.dpb_media],AL    ; Media byte
        LODSW                           ; Number of sectors in a FAT
        STOSW                           ;AC000;;>32mb dpb_FAT_size
        MOV     DL,BH                   ;AN000;;>32mb
        XOR     DH,DH                   ;AN000;;>32mb
        MUL     DX                      ;AC000;;>32mb Space occupied by all FATs
        ADD     AX,ES:[BP.dpb_first_FAT]
        STOSW                           ; dpb_dir_sector
        ADD     AX,CX                   ; Add number of directory sectors
        MOV     ES:[BP.dpb_first_sector],AX

        MOV     CL,BL                  ;F.C. >32mb                              ;AN000;
        CMP     WORD PTR ES:[BP.DSKSIZ],0       ;F.C. >32mb                     ;AN000;
        JNZ     normal_dpb             ;F.C. >32mb                              ;AN000;
        XOR     CH,CH                  ;F.C. >32mb                              ;AN000;
        MOV     BX,WORD PTR [SI+BPB_BigTotalSectors-BPB_SectorsPerTrack]        ;AN000;
        MOV     DX,WORD PTR [SI+BPB_BigTotalSectors-BPB_SectorsPerTrack+2]      ;AN000;
        SUB     BX,AX                  ;AN000;;F.C. >32mb
        SBB     DX,0                   ;AN000;;F.C. >32mb
        OR      CX,CX                  ;AN000;;F.C. >32mb
        JZ      norot                  ;AN000;;F.C. >32mb
rott:                                  ;AN000;;F.C. >32mb
        CLC                            ;AN000;;F.C. >32mb
        RCR     DX,1                   ;AN000;;F.C. >32mb
        RCR     BX,1                   ;AN000;;F.C. >32mb
        LOOP    rott                   ;AN000;;F.C. >32mb
norot:                                 ;AN000;
        MOV     AX,BX                  ;AN000;;F.C. >32mb
        JMP     setend                 ;AN000;;F.C. >32mb
normal_dpb:
        SUB     AX,ES:[BP.DSKSIZ]
        NEG     AX                      ; Sectors in data area
;;      MOV     CL,BL                   ; dpb_cluster_shift
        SHR     AX,CL                   ; Div by sectors/cluster
setend:
        INC     AX
        MOV     ES:[BP.dpb_max_cluster],AX
        MOV     ES:[BP.dpb_next_free],0 ; Init so first ALLOC starts at
                                        ; begining of FAT
        MOV     ES:[BP.dpb_free_cnt],-1 ; current count is invalid.
        return
EndProc $SETDPB
```

### 2.6 RBIL release 61 INTERRUP.F

```
--------D-211F-------------------------------
INT 21 - DOS 1+ - GET DRIVE PARAMETER BLOCK FOR DEFAULT DRIVE
        AH = 1Fh
Return: AL = status
            00h successful
                DS:BX -> Drive Parameter Block (DPB) (see #01357 for DOS 1.x,
                        AH=32h for DOS 2+)
            FFh invalid drive
Note:   this call was undocumented prior to the release of DOS 5.0; however,
          only the DOS 4.0+ version of the DPB has been documented
SeeAlso: AH=32h,AX=7302h

--------D-2132-------------------------------
INT 21 - DOS 2+ - GET DOS DRIVE PARAMETER BLOCK FOR SPECIFIC DRIVE
        AH = 32h
        DL = drive number (00h = default, 01h = A:, etc)
Return: AL = status
            00h successful
                DS:BX -> Drive Parameter Block (DPB) (see #01395) for specified
                          drive
            FFh invalid or network drive
Notes:  the OS/2 compatibility box supports the DOS 3.3 version of this call
          except for the DWORD at offset 12h
        this call updates the DPB by reading the disk; the DPB may be accessed
          via the DOS list of lists (see #01627 at AH=52h) if disk access is not
          desirable.
        undocumented prior to the release of DOS 5.0; only the DOS 4.0+
          version of the DPB has been documented, however
        supported by DR DOS 3.41+; DR DOS 3.41-6.0 return the same data as
          MS-DOS 3.31
        IBM ROM-DOS v4.0 also reports invalid/network (AL=FFh) on the ROM drive
SeeAlso: AH=1Fh,AH=52h,AX=7302h

Format of DOS Drive Parameter Block:
Offset  Size    Description     (Table 01395)
 00h    BYTE    drive number (00h = A:, 01h = B:, etc)
 01h    BYTE    unit number within device driver
 02h    WORD    bytes per sector
 04h    BYTE    highest sector number within a cluster
 05h    BYTE    shift count to convert clusters into sectors
 06h    WORD    number of reserved sectors at beginning of drive
 08h    BYTE    number of FATs
 09h    WORD    number of root directory entries
 0Bh    WORD    number of first sector containing user data
 0Dh    WORD    highest cluster number (number of data clusters + 1)
                16-bit FAT if greater than 0FF6h, else 12-bit FAT
 0Fh    BYTE    number of sectors per FAT
 10h    WORD    sector number of first directory sector
 12h    DWORD   address of device driver header (see #01646)
 16h    BYTE    media ID byte (see #01356)
 17h    BYTE    00h if disk accessed, FFh if not
 18h    DWORD   pointer to next DPB
---DOS 2.x---
 1Ch    WORD    cluster containing start of current directory, 0000h=root,
                FFFFh = unknown
 1Eh 64 BYTEs   ASCIZ pathname of current directory for drive
---DOS 3.x---
 1Ch    WORD    cluster at which to start search for free space when writing
 1Eh    WORD    number of free clusters on drive, FFFFh = unknown
---DOS 4.0-6.0---
 0Fh    WORD    number of sectors per FAT
 11h    WORD    sector number of first directory sector
 13h    DWORD   address of device driver header (see #01646)
 17h    BYTE    media ID byte (see #01356)
 18h    BYTE    00h if disk accessed, FFh if not
 19h    DWORD   pointer to next DPB
 1Dh    WORD    cluster at which to start search for free space when writing,
                usually the last cluster allocated
 1Fh    WORD    number of free clusters on drive, FFFFh = unknown
SeeAlso: #01357,#01663,#01787 at AX=7302h,#04039 at INT E0/CL=71h
```

## 3. The DOS 3.x DPB (32 bytes) -- the layout InitechDOS publishes

| Off | Size | Field | Source of the value (BPB -> DPB, $SETDPB 2.5) |
|---|---|---|---|
| 00h | BYTE | drive (0=A:) | the drive the DPB describes |
| 01h | BYTE | unit within the driver | driver's unit number |
| 02h | WORD | bytes per sector | BPB 0Bh |
| 04h | BYTE | highest sector in a cluster | BPB 0Dh - 1 (cluster mask) |
| 05h | BYTE | cluster->sector shift | log2(BPB 0Dh) |
| 06h | WORD | reserved sectors = first FAT sector | BPB 0Eh |
| 08h | BYTE | number of FATs | BPB 10h |
| 09h | WORD | root directory entries | BPB 11h |
| 0Bh | WORD | first data sector | dir_sector + ceil(root*32/bps) |
| 0Dh | WORD | highest cluster number | ((total - first data) >> shift) + 1 |
| 0Fh | BYTE | sectors per FAT | BPB 16h, stored with STOSB (3.x; WORD only from 4.0, `;AC000;`) |
| 10h | WORD | first root directory sector | reserved + nfats * spf |
| 12h | DWORD | device driver address | pointer to the block driver |
| 16h | BYTE | media descriptor | BPB 15h |
| 17h | BYTE | 00h accessed / FFh not yet accessed | FFh at init (MSINIT), 0 after FATREAD |
| 18h | DWORD | next DPB | chain; last = FFFF:FFFF (MSINIT `MOV AX,-1`) |
| 1Ch | WORD | cluster to start the free search at | 0 after $SETDPB ("first ALLOC starts at beginning of FAT") |
| 1Eh | WORD | free clusters, FFFFh = unknown | -1 after $SETDPB |

Call contract (GETSET.ASM 2.0 / MISC.ASM 4.0 / RBIL): AH=1Fh forces DL=0 and
falls into AH=32h. DL=0 is the default drive, 1=A:, 2=B:, ... . A valid
drive is media-checked (FATREAD) and returns AL=00h with DS:BX -> DPB; an
invalid drive returns AL=FFh and leaves DS:BX untouched. Neither path uses
CF as an error channel (the status is AL); InitechDOS clears CF on both, as
its AH=1Bh/1Ch FCB-family returns already do.

## 4. Mapping onto InitechDOS (flat 32-bit, DEC-04a)

- DS:BX becomes EBX = flat linear address of the DPB (the same collapse AH=1Bh
  and AH=35h already use; spec/int21h_calling_convention.json abi).
- Pointer DWORDs (12h, 18h) are flat 32-bit addresses. The chain end FFFF:FFFF
  becomes 0xFFFFFFFF. DEC-17's kernel block service has no installable block
  device header, so 12h is 0 (no driver header to point at) -- stated, not
  invented.
- The resident block service numbers units by drive letter (A=0, C=2, D=3),
  as IBMBIO's built-in driver does for its own drives.
- Sector numbers (06h, 0Bh, 10h) are volume-relative, as in DOS, where the
  driver adds the partition's hidden sectors (DEC-07a; fat12.h base_lba).
- Every mounted volume is read at mount (BPB + FAT), so 17h is 00h.
- InitechDOS keeps no free-search rover; allocation scans from the start of
  the FAT, which is exactly what 1Ch=0 means.
- 1Eh: the requested drive's free count is computed on the call (the same
  FAT walk AH=36h does); a DPB in the chain that has never been requested
  reports FFFFh (unknown), as DOS does until it has counted.

## 5. Oracle independence (Law 2)

The gate compares the published bytes against `minfo` (mtools) and `mdir`
for the same images, with the field table above transcribed into the shell
oracle independently of spec/dos_structs.h and of fat12.c's geometry. A wrong
first-data-sector (named mutant) must go RED there.
