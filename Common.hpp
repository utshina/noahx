#pragma once

constexpr uint64_t GiB = 1024*1024*1024;
constexpr int PAGE_SIZE_4K = 1024*1024;

constexpr uint64_t
rounddown(uint64_t value, size_t size)
{
        return (value & (~(size - 1)));
}

constexpr inline uint64_t
roundup(uint64_t value, size_t size)
{
        return rounddown(value + size - 1, size);
}
