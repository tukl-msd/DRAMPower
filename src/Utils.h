/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#ifndef UTILS_H
#define	UTILS_H

#include <string>
#include <sstream>
#include <typeinfo>
#include <stdexcept>

#define MILLION 1000000

template<typename T>
T fromString(const std::string& s,
        std::ios_base& (*f)(std::ios_base&) = std::dec)
throw (std::runtime_error) {
    std::istringstream is(s);
    T t;
    if (!(is >> f >> t)) {
        throw std::runtime_error("Cannot convert '" + s + "' to " +
                typeid (t).name() + " using fromString");

    }

    return t;
}

#endif	/* UTILS_H */