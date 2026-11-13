# Conditional mutation hooks awaiting patches

243 guarded source locations; 202 distinct macros.
One row per guard location; repeated macros must be converted together.
Breakage is named by the legacy macro, with the intended catcher below. Only
the three controls in ACCEPTANCE.txt were rerun for this import; this catalogue
is not a claim that the entire old mutation sweep is green. Hooks confined to
test drivers or generators and the parked desktop are outside this count.

| Location | Macro | Deliberate breakage | Intended catcher (test_ prefix) |
|---|---|---|---|
| blitter.c:85 | BLITTER_MUTATE_IGNORE_CLIP | blitter mutate ignore clip | blitter |
| blitter.c:130 | BLITTER_MUTATE_OFF_BY_ONE | blitter mutate off by one | blitter |
| blitter.c:190 | BLITTER_MUT_COPY32_ZERO | blitter mut copy32 zero | blitter |
| blitter.c:208 | BLITTER_MUTATE_OFF_BY_ONE | blitter mutate off by one | blitter |
| blitter.c:222 | BLITTER_MUT_COPY32_ZERO | blitter mut copy32 zero | blitter |
| chrome.c:152 | CHROME_FID_MUT_DARK_RING_BLACK | chrome fid mut dark ring black | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:171 | CHROME_FID_MUT_BOX_GEOM | chrome fid mut box geom | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:177 | CHROME_MUT_PRESSED_IDLE | chrome mut pressed idle | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:182 | CHROME_FID_MUT_RAMP | chrome fid mut ramp | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:213 | CHROME_FID_MUT_WIDGET_CORNERS_EDGE | chrome fid mut widget corners edge | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:228 | CHROME_FID_MUT_RAMP | chrome fid mut ramp | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:241 | CHROME_FID_MUT_ZOOM_CENTERED | chrome fid mut zoom centered | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:265 | CHROME_FID_MUT_CBOX_CORNER | chrome fid mut cbox corner | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:343 | CHROME_MUT_TRUNC_PERIOD | chrome mut trunc period | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:369 | CHROME_FID_MUT_CENTER_OFF | chrome fid mut center off | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:390 | CHROME_FID_MUT_NO_INACTIVE | chrome fid mut no inactive | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:393 | CHROME_MUTATE_TITLEBAR_H | chrome mutate titlebar h | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:398 | CHROME_FID_MUT_NO_BEVEL | chrome fid mut no bevel | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:414 | CHROME_FID_MUT_PHASE | chrome fid mut phase | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:420 | CHROME_FID_MUT_STRIPE_UNSHIFTED | chrome fid mut stripe unshifted | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:432 | CHROME_MUTATE_TITLEBAR_H | chrome mutate titlebar h | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:442 | CHROME_FID_MUT_INACTIVE_BLACK_FRAME | chrome fid mut inactive black frame | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:468 | CHROME_FID_MUT_INACTIVE_BRIGHT_TITLE | chrome fid mut inactive bright title | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:474 | CHROME_MUT_TITLE_UNCLIPPED | chrome mut title unclipped | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:487 | CHROME_FID_MUT_NO_TITLE | chrome fid mut no title | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:487 | CHROME_FID_MUT_TITLE_BLANK | chrome fid mut title blank | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:513 | CHROME_FID_MUT_SCROLL_FLAT | chrome fid mut scroll flat | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:537 | SB_MUT_TILE_FLAT | sb mut tile flat | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:542 | SB_LOOK_MUT_CORNERS_BEVELLED | sb look mut corners bevelled | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:560 | SB_MUT_FLAT_WELL | sb mut flat well | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:609 | CHROME_MUT_SCROLL_ALWAYS_ENABLED | chrome mut scroll always enabled | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:622 | CHROME_FID_MUT_SCROLL_FLAT | chrome fid mut scroll flat | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:653 | SB_LOOK_MUT_RAMP_FLAT | sb look mut ramp flat | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:667 | SB_LOOK_MUT_GRIPS_MISSING | sb look mut grips missing | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:691 | CHROME_MUT_SCROLL_PRESSED_IDLE | chrome mut scroll pressed idle | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:709 | CHROME_FID_MUT_SCROLL_FLAT | chrome fid mut scroll flat | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:743 | SB_MUT_SEP_GRAY | sb mut sep gray | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:787 | CHROME_MUT_GROW_OMIT | chrome mut grow omit | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:790 | CHROME_MUT_GROW_INACTIVE_ACTIVE | chrome mut grow inactive active | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:793 | CHROME_MUT_GROW_FIGURE_SWAP | chrome mut grow figure swap | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:831 | CHROME_FID_MUT_BODYBAR | chrome fid mut bodybar | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:897 | CHROME_FID_MUT_BOX_GEOM | chrome fid mut box geom | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:969 | CHROME_FID_MUT_NO_INACTIVE | chrome fid mut no inactive | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:978 | CHROME_FID_MUT_INACTIVE_BLACK_FRAME | chrome fid mut inactive black frame | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:985 | CHROME_FID_MUT_BODYBAR | chrome fid mut bodybar | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1001 | FLAIR_COLORBLIND_MUTANT | flair colorblind mutant | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1018 | CHROME_MUTATE_SCROLLBAR_W | chrome mutate scrollbar w | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1023 | CHROME_FID_MUT_NOTCH_DOUBLE | chrome fid mut notch double | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1035 | CHROME_MUT_GROW_NO_REGION | chrome mut grow no region | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1046 | CHROME_FID_MUT_KEEP_GADGETS | chrome fid mut keep gadgets | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1051 | CHROME_FID_MUT_BOX_GEOM | chrome fid mut box geom | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1068 | CHROME_FID_MUT_COLLAPSE | chrome fid mut collapse | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1077 | CHROME_MUTATE_NO_FRAME | chrome mutate no frame | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1086 | CHROME_FID_MUT_NO_SHADOW | chrome fid mut no shadow | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1088 | CHROME_FID_MUT_NOTCH | chrome fid mut notch | chrome_fidelity / chrome / scrollbar_look |
| chrome.c:1204 | CHROME_MUT_RDOC_NO_CLOSE | chrome mut rdoc no close | chrome_fidelity / chrome / scrollbar_look |
| control.c:68 | CTRL_MUT_NULL_SILENT | ctrl mut null silent | control / scrollbar_look |
| control.c:261 | CONTROL_MUTATE_NO_CLAMP | control mutate no clamp | control / scrollbar_look |
| control.c:325 | CONTROL_MUTATE_THUMB_OFF | control mutate thumb off | control / scrollbar_look |
| control.c:367 | CONTROL_MUTATE_THUMB_OFF | control mutate thumb off | control / scrollbar_look |
| control.c:481 | CTRL_MUT_PRESS_ACCENT | ctrl mut press accent | control / scrollbar_look |
| control.c:487 | CTRL_MUT_PRESS_ACCENT | ctrl mut press accent | control / scrollbar_look |
| control.c:501 | CTRL_MUT_BTN_FLAT | ctrl mut btn flat | control / scrollbar_look |
| control.c:578 | CTRL_MUT_BTN_FRINGE | ctrl mut btn fringe | control / scrollbar_look |
| control.c:608 | CTRL_MUT_LABEL_OPAQUE | ctrl mut label opaque | control / scrollbar_look |
| control.c:612 | CTRL_MUT_PRESS_ACCENT | ctrl mut press accent | control / scrollbar_look |
| control.c:667 | CTRL_MUT_CHECK_FILL | ctrl mut check fill | control / scrollbar_look |
| control.c:724 | CTRL_MUT_RADIO_DESKTOP_BG | ctrl mut radio desktop bg | control / scrollbar_look |
| control.c:786 | SB_MUT_TILE_FLAT | sb mut tile flat | control / scrollbar_look |
| control.c:791 | SB_LOOK_MUT_CORNERS_BEVELLED | sb look mut corners bevelled | control / scrollbar_look |
| control.c:806 | SB_MUT_FLAT_WELL | sb mut flat well | control / scrollbar_look |
| control.c:841 | SB_LOOK_MUT_RAMP_FLAT | sb look mut ramp flat | control / scrollbar_look |
| control.c:854 | SB_LOOK_MUT_GRIPS_MISSING | sb look mut grips missing | control / scrollbar_look |
| control.c:878 | SB_MUT_DISABLED_ENABLED_LOOK | sb mut disabled enabled look | control / scrollbar_look |
| control.c:914 | SB_MUT_SEP_GRAY | sb mut sep gray | control / scrollbar_look |
| control.c:969 | CTRL_MUT_PROGRESS_FLAT | ctrl mut progress flat | control / scrollbar_look |
| control.c:1037 | CTRL_MUT_NO_DEFAULT_RING | ctrl mut no default ring | control / scrollbar_look |
| control.c:1084 | CTRL_MUT_RING_OLD_CORNERS | ctrl mut ring old corners | control / scrollbar_look |
| control.c:1153 | CTRL_MUT_DISABLED_HIT | ctrl mut disabled hit | control / scrollbar_look |
| control.c:1182 | CTRL_MUT_SHORT_BUTTON | ctrl mut short button | control / scrollbar_look |
| control.c:1185 | CTRL_MUT_SHORT_LIVE | ctrl mut short live | control / scrollbar_look |
| control.c:1238 | CTRL_MUT_DISABLED_HIT | ctrl mut disabled hit | control / scrollbar_look |
| cp_service.h:14 | CP_MUT_MOUSE_INERT | cp mut mouse inert | deferred Control Panels smoke |
| cursor.c:32 | CURSOR_MUT_SQUARED_STEM | cursor mut squared stem | deferred icon/font smokes |
| cursor.c:50 | CURSOR_MUT_HOTSPOT | cursor mut hotspot | deferred icon/font smokes |
| cursor.c:199 | CURSOR_MUT_NO_ERASE | cursor mut no erase | deferred icon/font smokes |
| cursor.c:226 | CURSOR_MUT_BUSY_ARROW | cursor mut busy arrow | deferred icon/font smokes |
| dialog.c:402 | DIALOG_MUT_NO_ICON | dialog mut no icon | dialog / dialog_tracking |
| dialog.c:425 | DIALOG_MUT_NO_ICON | dialog mut no icon | dialog / dialog_tracking |
| dialog.c:449 | DIALOG_MUT_ALERT_SLAB | dialog mut alert slab | dialog / dialog_tracking |
| dialog.c:451 | DIALOG_MUTATE_BORDER | dialog mutate border | dialog / dialog_tracking |
| dialog.c:466 | DIALOG_MUT_ALERT_SLAB | dialog mut alert slab | dialog / dialog_tracking |
| dialog.c:477 | DIALOG_MUT_ALERT_SLAB | dialog mut alert slab | dialog / dialog_tracking |
| dialog.c:816 | DIALOG_MUT_ON_PRESS | dialog mut on press | dialog / dialog_tracking |
| dialog.c:865 | DIALOG_MUTATE_LOCAL_HIT | dialog mutate local hit | dialog / dialog_tracking |
| dialog.c:872 | DIALOG_MUTATE_HIT_STATIC | dialog mutate hit static | dialog / dialog_tracking |
| dialog.c:878 | DIALOG_MUT_ON_PRESS | dialog mut on press | dialog / dialog_tracking |
| dialog.c:1011 | DIALOG_MUTATE_FILECOPY_MSG | dialog mutate filecopy msg | dialog / dialog_tracking |
| dialog.c:1057 | DIALOG_MUTATE_PROGRESS_ZERO | dialog mutate progress zero | dialog / dialog_tracking |
| dialog.c:1088 | DIALOG_MUTATE_TITLELESS_MODAL | dialog mutate titleless modal | dialog / dialog_tracking |
| dialog.c:1107 | DIALOG_MUTATE_TITLELESS_MODAL | dialog mutate titleless modal | dialog / dialog_tracking |
| event.c:159 | RECORD_MUT_IGNORE_DBLCLICK_DEFINE | record mut ignore dblclick define | event |
| event.c:360 | RECORD_MUT_IGNORE_DBLCLICK_DEFINE | record mut ignore dblclick define | event |
| event.c:388 | EVENT_MUT_SILENT_DROP | event mut silent drop | event |
| event.c:467 | EVENT_MUT_NO_KEY_OBSERVER | event mut no key observer | event |
| event.c:486 | EVENT_MUTATE_SET1_BREAK_BLIND | event mutate set1 break blind | event |
| event.c:527 | EVENT_MUTATE_DROP_SYNTH | event mutate drop synth | event |
| event.c:580 | EVENT_MUTATE_STALE_WHERE | event mutate stale where | event |
| event.c:709 | EVENT_MUT_TICK_WRAP | event mut tick wrap | event |
| flair_look.c:84 | FLAIR_PART_MAP_MUT_DERIVED | flair part map mut derived | flair_part_map / skin_teal |
| flair_look.c:257 | FLAIR_MUT_SKIN_WRONG_ERA | flair mut skin wrong era | flair_part_map / skin_teal |
| heap.c:89 | FLAIR_HEAP_MUTATE_ALIGN_OVERFLOW | flair heap mutate align overflow | flair_heap |
| heap.c:94 | FLAIR_HEAP_MUTATE_SPAN_OVERFLOW | flair heap mutate span overflow | flair_heap |
| heap.c:101 | FLAIR_HEAP_MUTATE_NO_REUSE | flair heap mutate no reuse | flair_heap |
| heap.c:104 | FLAIR_HEAP_MUTATE_WALK_REUSE | flair heap mutate walk reuse | flair_heap |
| heap.c:143 | FLAIR_HEAP_MUTATE_NO_BOUNDS | flair heap mutate no bounds | flair_heap |
| include/asset_lz.h:48 | ASSET_LZ_MUT_BYTE | asset lz mut byte | asset_lz |
| include/assets/chicago12.h:192 | CHICAGO_MUT_ONE_ADVANCE | chicago mut one advance | deferred icon/font smokes |
| include/chicago_packed.h:15 | CHICAGO_PACK_MUT_ROW | chicago pack mut row | chicago_pack |
| include/control.h:216 | SB_MUT_THUMB_16 | sb mut thumb 16 | control / scrollbar_look |
| include/geneva12_packed.h:23 | GENEVA12_PACK_MUT_ROW | geneva12 pack mut row | geneva12_pack |
| include/menu.h:130 | MENU_LOOK_MUT_WRONG_RPAD | menu look mut wrong rpad | menu / menu_look |
| include/surface.h:88 | SURFACE_MUT_BGR_SWAP | surface mut bgr swap | blitter |
| include/surface.h:109 | SURFACE_MUT_INDEX_ZERO | surface mut index zero | blitter |
| include/system_key.h:13 | FLAIR_MUT_PLAIN_BSL_LAUNCH | flair mut plain bsl launch | deferred icon/font smokes |
| include/text.h:203 | TEXT_MUTATE_FIXED_PITCH | text mutate fixed pitch | text / font_styles / text_clip |
| include/text.h:296 | TEXT_MUTATE_FIXED_PITCH | text mutate fixed pitch | text / font_styles / text_clip |
| include/text.h:441 | TEXT_MUT_ITALIC_RUN4 | text mut italic run4 | text / font_styles / text_clip |
| include/text.h:462 | TEXT_MUT_SMEAR_OFF | text mut smear off | text / font_styles / text_clip |
| include/text.h:501 | TEXT_MUT_BOLD_ADVANCE_PLAIN | text mut bold advance plain | text / font_styles / text_clip |
| include/text.h:574 | TEXT_MUT_CLIP_IGNORE | text mut clip ignore | text / font_styles / text_clip |
| include/text.h:639 | TEXT_MUT_UL_OFFSET | text mut ul offset | text / font_styles / text_clip |
| include/winscroll.h:220 | WSCROLL_MUT_THUMB_UNSCALED | wscroll mut thumb unscaled | winscroll |
| include/winscroll.h:288 | WSCROLL_MUT_PAGE_WRONG_DIR | wscroll mut page wrong dir | winscroll |
| list.c:127 | LIST_MUT_ROW_WRAP | list mut row wrap | list |
| list.c:133 | LIST_MUT_NO_OVERFLOW_CHECK | list mut no overflow check | list |
| list.c:138 | LIST_MUT_COLUMN_HEADROOM | list mut column headroom | list |
| list.c:173 | LIST_MUT_CELL_INDEX_SWAP | list mut cell index swap | list |
| list.c:233 | LIST_MUT_NULL_CLICK_MISS | list mut null click miss | list |
| list.c:251 | LIST_MUT_HIT_OFFBYONE | list mut hit offbyone | list |
| list.c:281 | LIST_MUT_NO_DESELECT | list mut no deselect | list |
| menu.c:147 | MENU_MUTATE_FIXED_WIDTH | menu mutate fixed width | menu / menu_look |
| menu.c:212 | MENU_MUT_APPLE_DEAD | menu mut apple dead | menu / menu_look |
| menu.c:260 | MENU_MUTATE_FIXED_WIDTH | menu mutate fixed width | menu / menu_look |
| menu.c:373 | MENU_MUTATE_SELECT_DISABLED | menu mutate select disabled | menu / menu_look |
| menu.c:515 | MENU_MUT_NO_REHIT | menu mut no rehit | menu / menu_look |
| menu.c:521 | MENU_MUT_TRACK_CAP | menu mut track cap | menu / menu_look |
| menu.c:567 | MENU_MUT_BAR_FLAT | menu mut bar flat | menu / menu_look |
| menu.c:627 | MENU_LOOK_MUT_FLAT_HILITE | menu look mut flat hilite | menu / menu_look |
| menu.c:631 | MENU_MUT_BAR_FLAT | menu mut bar flat | menu / menu_look |
| menu.c:655 | MENU_MUT_BAR_FLAT | menu mut bar flat | menu / menu_look |
| menu.c:659 | MENU_MUT_TITLE_NO_HILITE | menu mut title no hilite | menu / menu_look |
| menu.c:687 | MENU_MUT_APPLE_SQUARE | menu mut apple square | menu / menu_look |
| menu.c:711 | MENU_LOOK_MUT_FLAT_HILITE | menu look mut flat hilite | menu / menu_look |
| menu.c:768 | MENU_MUT_NO_PANEL_BEVEL | menu mut no panel bevel | menu / menu_look |
| menu.c:813 | MENU_MUT_DISABLED_NORMAL_INK | menu mut disabled normal ink | menu / menu_look |
| menu.c:832 | MENU_LOOK_MUT_V_MARK | menu look mut v mark | menu / menu_look |
| menu.c:865 | MENU_MUT_NO_PANEL_BEVEL | menu mut no panel bevel | menu / menu_look |
| menu.c:895 | MENU_LOOK_MUT_OLD_DIVIDER_INSET | menu look mut old divider inset | menu / menu_look |
| menu.c:900 | MENU_MUT_SEP_PLAIN | menu mut sep plain | menu / menu_look |
| process.c:185 | BUDGET_MUT_OVERCOMMIT | budget mut overcommit | process / process_activate / process_teardown / process_update |
| process.c:234 | BUDGET_MUT_OVERCOMMIT | budget mut overcommit | process / process_activate / process_teardown / process_update |
| process.c:320 | PROC_MUT_ADMIT_IS_FOREGROUND | proc mut admit is foreground | process / process_activate / process_teardown / process_update |
| process.c:429 | TEARDOWN_MUT_LEAK_BLOCK | teardown mut leak block | process / process_activate / process_teardown / process_update |
| process.c:439 | TEARDOWN_MUT_LEAK_RECORDS | teardown mut leak records | process / process_activate / process_teardown / process_update |
| process.c:450 | PROC_MUT_STALE_MAGIC | proc mut stale magic | process / process_activate / process_teardown / process_update |
| process.c:475 | PROC_MUT_REGISTERED_FREE | proc mut registered free | process / process_activate / process_teardown / process_update |
| process.c:536 | FLAIR_LIVE_MUTATE_IGNORE_REFCON | flair live mutate ignore refcon | process / process_activate / process_teardown / process_update |
| process.c:612 | FLAIR_LIVE_MUTATE_SKIP_ACTIVATE | flair live mutate skip activate | process / process_activate / process_teardown / process_update |
| process.c:678 | PROC_MUT_FRONT_STAYS_ACTIVE | proc mut front stays active | process / process_activate / process_teardown / process_update |
| process.c:750 | FLAIR_LIVE_MUTATE_NO_RAISE_ON_TITLE | flair live mutate no raise on title | process / process_activate / process_teardown / process_update |
| process.c:799 | FLAIR_LIVE_MUTATE_NO_RAISE_ON_TITLE | flair live mutate no raise on title | process / process_activate / process_teardown / process_update |
| process.c:838 | PROC_MUT_CLICK_THROUGH | proc mut click through | process / process_activate / process_teardown / process_update |
| resource.c:84 | RES_MUT_COUNT_OFF_BY_ONE | res mut count off by one | resource |
| resource.c:100 | RES_MUT_IGNORE_TYPE | res mut ignore type | resource |
| resource.c:150 | RES_MUT_MAP_PREAMBLE | res mut map preamble | resource |
| resource.c:168 | RES_MUT_TYPE_WRAP | res mut type wrap | resource |
| resource.c:189 | RES_MUT_DATA_WRAP | res mut data wrap | resource |
| resource.c:196 | RES_MUT_DATA_ESCAPE | res mut data escape | resource |
| resource.c:196 | RES_MUT_DATA_WRAP | res mut data wrap | resource |
| scrap.c:76 | SCRAP_MUT_IGNORE_FLAVOR | scrap mut ignore flavor | scrap |
| scrap.c:121 | SCRAP_MUT_NO_CLEAR | scrap mut no clear | scrap |
| scrap.c:124 | SCRAP_MUT_NO_INCREMENT | scrap mut no increment | scrap |
| scrap.c:142 | SCRAP_MUT_NULL_DATA | scrap mut null data | scrap |
| scrap.c:161 | SCRAP_MUT_FULL_FLAVOR | scrap mut full flavor | scrap |
| stdfile.c:356 | SF_MUT_NO_NAVIGATE | sf mut no navigate | stdfile |
| stdfile.c:409 | SF_MUT_RETURN_WRONG_CELL | sf mut return wrong cell | stdfile |
| stdfile.c:433 | SF_MUT_SWALLOW_NAV_ERROR | sf mut swallow nav error | stdfile |
| stdfile.c:460 | SF_MUT_CANCEL_GOOD | sf mut cancel good | stdfile |
| surface.c:27 | SURFACE_MUT_CLEAR_FLAT | surface mut clear flat | blitter |
| tbxfile.h:15 | TBX_MUT_FILE_BOUNDS | tbx mut file bounds | tbx_file |
| tbxfile.h:45 | TBX_MUT_FILE_RENAME_EDI | tbx mut file rename edi | tbx_file |
| tbxfile.h:48 | TBX_MUT_FILE_OWNER | tbx mut file owner | tbx_file |
| tbxfile.h:58 | TBX_MUT_FILE_CREATE_NEW_TRUNC | tbx mut file create new trunc | tbx_file |
| tbxfile.h:67 | TBX_MUT_FILE_OWNER | tbx mut file owner | tbx_file |
| tbxfile.h:102 | TBX_MUT_FILE_WRITE_NOOP | tbx mut file write noop | tbx_file |
| tbxfile.h:127 | TBX_MUT_FILE_LEAK | tbx mut file leak | tbx_file |
| tbxgate.c:278 | TBX_MUT_NO_SLOT_GUARD | tbx mut no slot guard | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:424 | TBX_MUT_NO_SLOT_GUARD | tbx mut no slot guard | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:538 | TBX_MUT_TOKEN_ALIAS | tbx mut token alias | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:646 | TBX_MUT_GENEVA_IGNORED | tbx mut geneva ignored | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:695 | TBXGATE_MUT_CELL_UL_LOW | tbxgate mut cell ul low | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:771 | TBX_MUT_MBAR_NO_BOUNDS | tbx mut mbar no bounds | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:841 | TBX_MUT_MBAR_NOT_SWAPPED | tbx mut mbar not swapped | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:859 | TBX_MUT_DRAWMBAR_NOOP | tbx mut drawmbar noop | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:1152 | TBX_MUT_AX_FULL_EAX | tbx mut ax full eax | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:1207 | TBX_MUT_ARG_OFFSET | tbx mut arg offset | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:1373 | TBX_MUT_EXIT_LEAK | tbx mut exit leak | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:1455 | TBX_MUT_DRAW_NOT_PRESENTED | tbx mut draw not presented | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:1475 | TBX_MUT_MENU_NOT_DELIVERED | tbx mut menu not delivered | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:1699 | TBX_MUT_FILECOPY_RATE_IGNORED | tbx mut filecopy rate ignored | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:1744 | TBX_MUT_FILECOPY_NO_COPY | tbx mut filecopy no copy | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxgate.c:1760 | TBX_MUT_FILECOPY_NO_COPY | tbx mut filecopy no copy | flair_routing; deferred tenant smokes for unexercised verbs |
| tbxmenu.h:60 | TBX_MUT_MENU_KEY_PLAIN | tbx mut menu key plain | tbx_menu |
| tbxmenu.h:84 | TBX_MUT_MENU_ANY_BAR | tbx mut menu any bar | tbx_menu |
| textedit.c:174 | TE_MUT_LINEBREAK_OFF | te mut linebreak off | textedit / te_surface |
| textedit.c:188 | TE_MUT_WRAP_CHARS | te mut wrap chars | textedit / te_surface |
| textedit.c:248 | TE_MUT_SEL_INCLUSIVE | te mut sel inclusive | textedit / te_surface |
| textedit.c:315 | TE_MUT_HIT_PAST_CR | te mut hit past cr | textedit / te_surface |
| textedit.c:338 | TE_MUT_HIT_FLOOR | te mut hit floor | textedit / te_surface |
| textedit.c:357 | TE_MUT_ARROW_NO_COLLAPSE | te mut arrow no collapse | textedit / te_surface |
| textedit.c:417 | TE_MUT_EXTEND_ANCHOR | te mut extend anchor | textedit / te_surface |
| textedit.c:448 | TE_MUT_PROBE_NO_LINES | te mut probe no lines | textedit / te_surface |
| textedit.c:562 | TE_MUT_PASTE_NO_DELETE | te mut paste no delete | textedit / te_surface |
| textedit.c:574 | TE_MUT_PASTE_NO_DELETE | te mut paste no delete | textedit / te_surface |
| window.c:147 | CHROME_FID_MUT_SHADOW_CLIPPED | chrome fid mut shadow clipped | window / interact / winscroll |
| window.c:182 | CHROME_FID_MUT_SHADOW_CLIPPED | chrome fid mut shadow clipped | window / interact / winscroll |
| window.c:194 | CHROME_FID_MUT_CONTENT_TOP_STALE | chrome fid mut content top stale | window / interact / winscroll |
| window.c:200 | CHROME_FID_MUT_CONTENT_LEFT_THIN | chrome fid mut content left thin | window / interact / winscroll |
| window.c:221 | WINDOW_MUTATE_CONTENT_OVER_HSCROLL | window mutate content over hscroll | window / interact / winscroll |
| window.c:242 | CHROME_FID_MUT_CONTENT_OVER_SCROLL | chrome fid mut content over scroll | window / interact / winscroll |
| window.c:280 | WINDOW_MUTATE_ZORDER | window mutate zorder | window / interact / winscroll |
| window.c:293 | WINDOW_MUTATE_IGNORE_OVERLAY | window mutate ignore overlay | window / interact / winscroll |
| window.c:486 | WINDOW_MUTATE_NO_DEACT_INVAL | window mutate no deact inval | window / interact / winscroll |
| window.c:490 | WINDOW_MUTATE_NO_ACTIVATE_INVAL | window mutate no activate inval | window / interact / winscroll |
| window.c:532 | WINDOW_MUTATE_OVERPAINT | window mutate overpaint | window / interact / winscroll |
| window.c:874 | WINDOW_MUTATE_COLLAPSE_LEAK | window mutate collapse leak | window / interact / winscroll |
| window.c:881 | WINDOW_MUT_SCROLL_STALE_RESIZE | window mut scroll stale resize | window / interact / winscroll |
| window.c:940 | WINDOW_MUTATE_GROW_NO_MIN | window mutate grow no min | window / interact / winscroll |
| window.c:955 | WINDOW_MUT_GROW_NO_COMMIT | window mut grow no commit | window / interact / winscroll |
| window.c:997 | WINDOW_MUTATE_ZOOM_NO_RESTORE | window mutate zoom no restore | window / interact / winscroll |
| window.c:1106 | WINDOW_MUTATE_SCROLL_DRAG | window mutate scroll drag | window / interact / winscroll |
| window.c:1125 | WINDOW_MUTATE_ZONE_OFF | window mutate zone off | window / interact / winscroll |
