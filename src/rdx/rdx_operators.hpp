#ifndef __RDX_OPERATORS_HPP_
#define __RDX_OPERATORS_HPP__

template<class TReturn, class TLeft, class TRight>
class rdxCOpProxyAdd { public: inline static TReturn Operate(TLeft left, TRight right) { return left + right; } };

template<class TReturn, class TLeft, class TRight>
class rdxCOpProxySub { public: inline static TReturn Operate(TLeft left, TRight right) { return left - right; } };

template<class TReturn, class TLeft, class TRight>
class rdxCOpProxyMul { public: inline static TReturn Operate(TLeft left, TRight right) { return left * right; } };

template<class TReturn, class TLeft, class TRight>
class rdxCOpProxyDiv { public: inline static TReturn Operate(TLeft left, TRight right) { return left / right; } };

template<class TReturn, class TLeft, class TRight>
class rdxCOpProxyMod { public: inline static TReturn Operate(TLeft left, TRight right) { return left % right; } };

template<class TOpReturn, class TLeft, class TRight>
class rdxCOpProxyLT { public: inline static TOpReturn Operate(TLeft left, TRight right) { return left < right; } };

template<class TOpReturn, class TLeft, class TRight>
class rdxCOpProxyGT { public: inline static TOpReturn Operate(TLeft left, TRight right) { return left > right; } };

template<class TOpReturn, class TLeft, class TRight>
class rdxCOpProxyLE { public: inline static TOpReturn Operate(TLeft left, TRight right) { return left <= right; } };

template<class TOpReturn, class TLeft, class TRight>
class rdxCOpProxyGE { public: inline static TOpReturn Operate(TLeft left, TRight right) { return left >= right; } };

template<class TOpReturn, class TLeft, class TRight>
class rdxCOpProxyEqual { public: inline static TOpReturn Operate(TLeft left, TRight right) { return left == right; } };

template<class TOpReturn, class TLeft, class TRight>
class rdxCOpProxyNE { public: inline static TOpReturn Operate(TLeft left, TRight right) { return left != right; } };

template<class TReturn, class TVal>
class rdxCOpProxyNegate { public: inline static TReturn Operate(TVal val) { return -val; } };

#endif
