union RuntimeValue4
{
	UInt8 bytes[4];
	LargeInt addrs[1];
	Int32 i32;
	Float32 f32;
	EnumValue ev;
	void* p;
	LargeInt li;
	const void* cp;
	UInt32 u32;
inline RuntimeValue4 &operator =(const volatile RuntimeValue4 &rs)
{
	const volatile LargeInt *volAddress = rs.addrs;
	for(LargeInt i=0;i<1;i++)
		this->addrs[i] = volAddress[i];
	return *this;
}
};
union RuntimeValue8
{
	UInt8 bytes[8];
	LargeInt addrs[2];
	RuntimePointer<void> rtp;
	Int64 i64;
	Float64 f64;
	UInt64 u64;
inline RuntimeValue8 &operator =(const volatile RuntimeValue8 &rs)
{
	const volatile LargeInt *volAddress = rs.addrs;
	for(LargeInt i=0;i<2;i++)
		this->addrs[i] = volAddress[i];
	return *this;
}
};
inline LargeInt PackAddress(UInt8 v1, UInt8 v2, UInt8 v3, UInt8 v4)
{
	union { LargeInt addr; UInt8 b8[4]; } u;
	u.b8[0] = v1;
	u.b8[1] = v2;
	u.b8[2] = v3;
	u.b8[3] = v4;
	return u.addr;
}

