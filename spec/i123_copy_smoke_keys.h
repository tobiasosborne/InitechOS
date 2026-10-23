/* Deliberately added CPU-path replay, TBX_SMOKE_I123_COPY only (factory).
 * Real R2.2 fixtures/i123-copy: SINGLE, ROW, OVER, BLOCK. Same events on
 * QEMU and Bochs. Typed keys use ASCII, pointer keys cooked set-1 codes. */
'7','\r','/','c','a','1','\r','d','1','.','.','d','3','\r',
'/','f','s','s','i','n','g','l','e','\r',
'/','w','e','y',
'1','1',0x4d00,'2','2','\r',
'/','c','a','1','.','.','b','1','\r','f','1','.','.','h','3','\r',
'/','f','s','r','o','w','\r',
'/','c','a','1','.','.','b','1','\r','a','1','.','.','d','2','\r',
'/','f','s','o','v','e','r','\r',
'/','w','e','y',
'1',0x4d00,'2',0x5000,'4',0x4b00,'3','\r',
'/','c','a','1','.','.','b','2','\r','f','1','.','.','i','4','\r',
'/','f','s','b','l','o','c','k','\r',
0x3f00,'a','1','\r','/','c',27,0x4d00,'.',0x5000,0x4d00,27,
'.',0x5000,0x4d00,8,'.',0x5000,0x4d00
