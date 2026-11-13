/* Component input contract must match the actual Toolbox ABI. */
_Static_assert(IW_LF_REC_BYTES == TBX_FIND_REC_BYTES, "List Files record BYTES");
_Static_assert(IW_LF_REC_ATTR_OFF == TBX_FIND_REC_ATTR_OFF, "List Files record ATTR_OFF");
_Static_assert(IW_LF_REC_TIME_OFF == TBX_FIND_REC_TIME_OFF, "List Files record TIME_OFF");
_Static_assert(IW_LF_REC_DATE_OFF == TBX_FIND_REC_DATE_OFF, "List Files record DATE_OFF");
_Static_assert(IW_LF_REC_SIZE_OFF == TBX_FIND_REC_SIZE_OFF, "List Files record SIZE_OFF");
_Static_assert(IW_LF_REC_NAME_OFF == TBX_FIND_REC_NAME_OFF, "List Files record NAME_OFF");
