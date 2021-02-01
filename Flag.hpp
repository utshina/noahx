#pragma once

#include <type_traits>

#define ENUM_FLAG(T)								\
	constexpr T operator|(T l, T r)		\
	{					\
		using U = typename std::underlying_type<T>::type;	\
		return static_cast<T>(static_cast<U>(l) | static_cast<U>(r));	\
	}
