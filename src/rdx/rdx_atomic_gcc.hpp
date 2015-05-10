template<class T>
inline rdxAtomic<T>::rdxAtomic()
{
}

template<class T>
inline rdxAtomic<T>::rdxAtomic(const rdxAtomic<T> &other)
{
	WriteUnfenced(other.ReadUnfenced());
}

template<class T>
inline rdxAtomic<T> &rdxAtomic<T>::operator =(const rdxAtomic<T> &other)
{
	WriteUnfenced(other.ReadUnfenced());
	return *this;
}

template<class T>
T rdxAtomic<T>::ReadBeforeLater() const
{
	T result = m_value;
	__builtin_ia32_lfence();
	return result;
}


template<class T>
T rdxAtomic<T>::ReadFullFence() const
{
	__builtin_ia32_lfence();
	T result = m_value;
	__builtin_ia32_lfence();
	return result;
}

template<class T>
T rdxAtomic<T>::ReadUnfenced() const
{
	return m_value;
}

template<class T>
void rdxAtomic<T>::WriteAfterPrevious(T value)
{
	__builtin_ia32_sfence();
	m_value = T;
}

template<class T>
void rdxAtomic<T>::WriteBeforeLater(T value)
{
	m_value = T;
	__builtin_ia32_sfence();
}

template<class T>
void rdxAtomic<T>::WriteUnfenced(T value)
{
	m_value = T;
}

template<class T>
void rdxAtomic<T>::WriteFullFence(T value)
{
	__builtin_ia32_sfence();
	m_value = T;
	__builtin_ia32_sfence();
}

template<class T>
T rdxAtomic<T>::IncrementFullFence()
{
}

T IncrementUnfenced();

	T DecrementFullFence();
	T DecrementUnfenced();

	// Compares *exchangeAddr to compareTo, and if equal, sets it to replacement.  Returns the original value of *exchangeAddr
	T CompareExchangeFullFence(T replacement, T compareTo);
	T CompareExchangeUnfenced(T replacement, T compareTo);

	// Writes replacement, returns original value
	T ExchangeFullFence(T replacement);
	T ExchangeUnfenced(T replacement);
