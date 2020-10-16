#pragma once
#ifndef CALC_UTIL_WINDOWS_H
#define CALC_UTIL_WINDOWS_H

#include "pch.h"
#include <cassert>

namespace calc_util {

struct DSize : public winrt::Windows::Foundation::Size {
// variation of window's Size struct but with accessor functions that give
// double values instead of float values. this is to keep calculations in the
// double domain, especially since functions such as
// FrameworkElement::ActualHeight return double instead of float. float values
// are still accessable via base class
    using Size = winrt::Windows::Foundation::Size;
    DSize() : Size{0, 0} {}
    DSize(double width, double height);
    DSize(const Size& size) : Size{size} {}
    double width() const {return Width;}
    double height() const {return Height;}
    void width(double width) {Width = static_cast<float>(width); assert(static_cast<double>(Width) == width);}
    void height(double height) {Height = static_cast<float>(height); assert(static_cast<double>(Height) == height);}
    void width(float width) {Width = width;}
    void height(float height) {Height = height;}
};

inline DSize::DSize(double width, double height)
        : Size{static_cast<float>(width), static_cast<float>(height)} {
    assert(fabs(static_cast<double>(Size::Width) - width) < 0.001);
    assert(fabs(static_cast<double>(Size::Height) - height) < 0.001);
}

} // namespace calc_util

namespace winrt {

inline Windows::Foundation::IInspectable box_value(const calc_util::DSize& value) {
    return box_value(static_cast<const Windows::Foundation::Size&>(value));
}

inline calc_util::DSize
unbox_value_or(winrt::Windows::Foundation::IInspectable const& value, const calc_util::DSize& default_value) {
    return unbox_value_or(value, static_cast<const Windows::Foundation::Size&>(default_value));
}

} // namespace winrt

#endif // CALC_UTIL_WINDOWS_H