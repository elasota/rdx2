#ifndef __RDX_BREAKCAUSE_HPP__
#define __RDX_BREAKCAUSE_HPP__

enum rdxBreakCause
{
	rdxBREAKCAUSE_Exception,
	rdxBREAKCAUSE_UnusualCast,
	rdxBREAKCAUSE_SentinelAuditFailed,
	rdxBREAKCAUSE_UserBreak,
	rdxBREAKCAUSE_Unimplemented,
	rdxBREAKCAUSE_CriticalInternalFailure,
};

#endif
