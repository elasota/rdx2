#ifndef __RDX_DEBUGTYPEINSPECTOR_HPP__
#define __RDX_DEBUGTYPEINSPECTOR_HPP__


struct rdxIDebugTypeInspector
{
	virtual void VTableInsert() {}
};

template<class T>
struct rdxSDebugTypeInspector : public rdxIDebugTypeInspector
{
	virtual void VTableInsert() {}
	T **m_objectVPtr;
};

#endif
