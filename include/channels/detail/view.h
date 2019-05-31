#pragma once
#include "compatibility/compile_features.h"

namespace channels {
namespace detail {

// This class just keeps a pair of iterators.
template<typename Container>
class CHANNELS_NODISCARD view {
public:
	using container_type = Container;
	using value_type = typename Container::value_type;
	using pointer = typename Container::pointer;
	using reference =  typename Container::reference;
	using iterator = typename Container::iterator;
	using const_pointer = typename Container::const_pointer;
	using const_reference =  typename Container::const_reference;
	using const_iterator = typename Container::const_iterator;
	using difference_type = typename Container::difference_type;

	view() = default;
	explicit constexpr view(Container& container);

	CHANNELS_NODISCARD constexpr iterator begin();
	CHANNELS_NODISCARD constexpr iterator end();
	CHANNELS_NODISCARD constexpr const_iterator begin() const;
	CHANNELS_NODISCARD constexpr const_iterator end() const;
	CHANNELS_NODISCARD constexpr const_iterator cbegin() const;
	CHANNELS_NODISCARD constexpr const_iterator cend() const;

	CHANNELS_NODISCARD constexpr bool empty() const;

private:
	iterator begin_;
	iterator end_;
};

// implementation

template<typename Container>
constexpr view<Container>::view(Container& container)
	: begin_{container.begin()}
	, end_{container.end()}
{}

template<typename Container>
constexpr typename view<Container>::iterator view<Container>::begin()
{
	return begin_;
}

template<typename Container>
constexpr typename view<Container>::iterator view<Container>::end()
{
	return end_;
}

template<typename Container>
constexpr typename view<Container>::const_iterator view<Container>::begin() const
{
	return begin_;
}

template<typename Container>
constexpr typename view<Container>::const_iterator view<Container>::end() const
{
	return end_;
}

template<typename Container>
constexpr typename view<Container>::const_iterator view<Container>::cbegin() const
{
	return begin();
}

template<typename Container>
constexpr typename view<Container>::const_iterator view<Container>::cend() const
{
	return end();
}

template<typename Container>
constexpr bool view<Container>::empty() const
{
	return begin() == end();
}

} // namespace detail
} // namespace channels
