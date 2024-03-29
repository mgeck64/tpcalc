﻿#pragma once

#include "MainPage.g.h"
#include "..\rdcalc\calc_outputter.h"
#include "calc_util_windows.h"
#include <sstream>

namespace winrt::tpcalc::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
    private:
        using parser_type = ::tpcalc::parser<wchar_t>;
        using radices = typename parser_type::radices;
        using outputter_type = ::tpcalc::outputter<wchar_t>;
        using internal_error = ::tpcalc::internal_error;
        using parse_error = ::tpcalc::parse_error<wchar_t>;
        using Size = Windows::Foundation::Size;
        using DSize = ::tpcalc::DSize;
        parser_type parser;
        outputter_type outputter;

        void evaluate_input();

        bool size_values_valid = false;
        DSize default_page_size{0, 0};
        DSize initial_calcPanel_size{0, 0};
        DSize initial_input_size{0, 0};
        DSize initial_output_size{0, 0};
        double input_height_limit_when_XPanel_showing = 0;
        double output_height_limit_when_XPanel_showing = 0;

        DSize XPanel_hint_for_vars = {0, 200}; // width determined in initialization
        DSize XPanel_hint_for_help = {500, 1024};

        Windows::UI::Xaml::Controls::ToolTip outputTip;

        static constexpr double XPanel_margin_bottom = 16; // extra space for margin; done in code-behind because can't make this work in XAML
        static constexpr double XPanel_height_space_threshold = 32; // arbitrary threshold value
        static constexpr double min_in_out_down_scale_factor = 3;

        parser_type::string last_input;
        using last_inputs_type = std::vector<parser_type::string>;
        last_inputs_type last_inputs;
        static constexpr last_inputs_type::size_type max_last_inputs_size = 25; // to keep last_inputs from growing indefinitely
        last_inputs_type::size_type last_inputs_idx = 0;

        bool outputted_last_val = false;

        void set_output_to_last_val();
        void set_output_to(std::wstring_view text);
        void update_button_labels();
        void update_page_layout();
        void update_mode_menu();
        void update_mode_display();
        void update_integer_result_type_menu();
        void on_vars_changed();
        void TryResizeView(DSize size);
        void TryResizeView(double width, double height) {return TryResizeView(DSize(width, height));}
        void show_vars(std::wstring_view tag);
        void show_help(std::wstring_view tag, bool first_app_launch = false);
        template <typename XPanelHint> auto width_and_height(const XPanelHint& XPanel_hint, bool first_app_launch = false) -> DSize const;
        DSize max_page_size();

    public:
        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void page_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void page_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::SizeChangedEventArgs const& e);
        void mode_menu_Opening(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);
        void input_mode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void output_mode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void integer_result_type_menu_Opening(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);
        void integer_result_type_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void input_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void append_tag_to_input_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void output_Copy_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void vars_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void vars_hide_vars_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void help_menu_Opening(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);
        void help_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void help_quick_start_guide_basic_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_quick_start_guide_advanced_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_variables_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_numbers_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_fp_and_integer_arithmetic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_fp_arithmetic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_integer_arithmetic_and_bitwise_logic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_scientific_functions_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_statistical_functions_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_vector_arithmetic_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_operator_precedence_and_associativity_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_hide_help_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void min_in_out_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void output_PointerEntered(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& e);
    };
}

namespace winrt::tpcalc::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
