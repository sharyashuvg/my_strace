#pragma once
#include <array>
#include <string_view>
#include <algorithm>
#include <charconv>
#include <ranges>

constexpr std::string_view raw_syscall_names[] = 
{
#include "syscall_list.txt"

};

/*
generate at compile time the maximum syscall number from the raw_syscall_names array
by splitting each string at the first space and converting the first part to an integer
then returning the maximum integer found
*/
consteval size_t max_syscall_number() {
    
    auto v = raw_syscall_names | std::views::transform([](std::string_view name) {
        auto num_view = *(std::views::split(name, ' ').begin());
        long long num{};
        std::from_chars(num_view.begin(), num_view.end(), num);
        return num;}
    );

    return *std::max_element(v.begin(), v.end());
};


/*
generate at compile time a map from syscall number to syscall name
by splitting each string at the first space and converting the first part to an integer
then using that integer as the index in the array and the rest of the string as the
value in the array
Note that the array is initialized with "unknown_syscall" for all entries
to handle the case where a syscall number is not present in the raw_syscall_names array
*/
consteval std::array<std::string_view, max_syscall_number() + 1> generate_syscall_name_map() {
    

    std::array<std::string_view, max_syscall_number() + 1> syscall_map;
    
    std::ranges::fill(syscall_map, "unknown_syscall");

    for (auto name : raw_syscall_names)
    {
        size_t space_index = name.find(' ');
        long long num{};
        std::from_chars(name.data() , name.data() + space_index, num);
        syscall_map[num] = name.substr(space_index + 1, name.find(' ', space_index + 1) - space_index - 1);
    }

    return syscall_map;
}

constexpr auto syscall_name_map = generate_syscall_name_map();

/*
generate at compile time a map from syscall number to syscall aritary and name
by splitting each string at the first space and converting the first part to an integer
then using that integer as the index in the array and the rest of the string as the
value in the array
Note that the array is initialized with "..." for all entries
to handle the case where a syscall number is not present in the raw_syscall_names array
*/
consteval std::array<std::pair<int, std::string_view>, max_syscall_number() + 1> generate_syscall_aritary() {

    std::array<std::pair<int, std::string_view>, max_syscall_number()+ 1> syscall_map;
    
    std::ranges::fill(syscall_map, std::make_pair(0, "..."));

    for (auto name : raw_syscall_names)
    {
        size_t first_space_index = name.find(' ');
        size_t second_space_index = name.find(' ', first_space_index + 1);
        size_t third_space_index = name.find(' ', second_space_index + 1);
        long long aritary{};
        long long num{};
        std::from_chars(name.data() , name.data() + first_space_index, num);
        std::from_chars(name.data() + second_space_index + 1 , name.data() + second_space_index + 2 , aritary);
        syscall_map[num] = std::make_pair(aritary, name.substr(third_space_index + 1));
    }

    return syscall_map;
}


constexpr auto syscall_aritary_map = generate_syscall_aritary();
