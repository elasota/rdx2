#include "rdx_basictypes.hpp"

RDX_UTIL_DYNLIB_API bool rdxIsDigit(rdxChar c)
{
	return (c >= '0' && c <= '9');
}

RDX_UTIL_DYNLIB_API rdxFloat64 rdxStringToDouble(const rdxChar *chars, rdxLargeUInt len)
{
	const rdxChar *endpoint = chars + len;
	rdxFloat64 fv = 0.0;
	rdxInt32 decimalShift = 0;
	rdxChar lastC = 0;
	bool negateAll = false;

	if(chars != endpoint)
	{
		lastC = *chars++;
		if(lastC == '-')
			negateAll = true;
	}

	while(true)
	{
		if(!rdxIsDigit(lastC))
			break;
		if(negateAll)
			fv = fv * 10.0 - (lastC - '0');
		else
			fv = fv * 10.0 + (lastC - '0');

		if(chars == endpoint)
			break;
		lastC = *chars++;
	}

	if(lastC == '.')
	{
		while(true)
		{
			if(chars == endpoint)
				break;
			lastC = *chars++;
			if(!rdxIsDigit(lastC))
				break;
			if(negateAll)
				fv = fv * 10.0 - (lastC - '0');
			else
				fv = fv * 10.0 + (lastC - '0');
			decimalShift--;
		}
	}
	if(lastC == 'e')
	{
		rdxInt32 exponent = 0;
		bool negate;
		if(chars != endpoint)
		{
			lastC = *chars++;
			if(lastC == '-')
			{
				negate = true;
				if(chars != endpoint)
					lastC = *chars++;
			}

			while(true)
			{
				if(!rdxIsDigit(lastC))
					break;
				if(negate)
					exponent = exponent * 10 - (lastC - '0');
				else
					exponent = exponent * 10 + (lastC - '0');
				if(chars == endpoint)
					break;
				lastC = *chars++;
			}
		}

		decimalShift += exponent;

		if(decimalShift == 0x8000000)
			fv = 0.0;
		else if(decimalShift == 0)
		{
			// Nothing
		}
		else if(decimalShift < 0)
		{
			decimalShift = -decimalShift;
			rdxFloat64 power = 10.0;
			rdxFloat64 expMul = 1.0;
			for(int i=0;i<32;i++)
			{
				power *= power;
				if(decimalShift & (1 << i))
					expMul *= power;
			}
			fv /= expMul;
		}
		else
		{
			rdxFloat64 power = 10.0;
			rdxFloat64 expMul = 1.0;
			for(int i=0;i<32;i++)
			{
				power *= power;
				if(decimalShift & (1 << i))
					expMul *= power;
			}
			fv *= expMul;
		}
	}

	return fv;
}
