#pragma once

#include "MainPage.g.h"
#include "..\rdcalc\calc_outputter.h"
#include <sstream>

namespace winrt::tcalc::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
    private:
        using parser_type = calc_parser<wchar_t>;
        using radices = typename parser_type::radices;
        using outputter_type = calc_outputter<wchar_t>;
        using Size = Windows::Foundation::Size;
        parser_type parser;
        outputter_type outputter;

        static bool initializing_app;

        static constexpr std::streamsize out_precision = 15;

        void eval_input();

        bool size_values_valid = false;
        Size default_page_size{0, 0};
        Size initial_calcPanel_size{0, 0};
        Size initial_input_size{0, 0};
        Size initial_output_size{0, 0};

        parser_type::string last_input;
        using last_inputs_type = std::vector<parser_type::string>;
        last_inputs_type last_inputs;
        static constexpr last_inputs_type::size_type max_last_inputs_size = 25; // to keep last_inputs from growing indefinitely
        last_inputs_type::size_type last_inputs_idx = 0;

        bool auto_resizing_page = false;

        bool outputted_last_val = false;

        static auto make_size(double width, double height) -> Size;
        void set_output_to_last_val();
        double space_for_XPanel();
        void update_XPanel_button_labels(double space_for_XPanel_);
        void update_XPanel_button_labels() {update_XPanel_button_labels(space_for_XPanel());}
        void update_page_ui();
        void update_mode_menu();
        void update_mode_display();
        void update_integer_result_type_menu();
        void on_vars_changed();
        void TryResizeView(Size size);

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
        void page_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void input_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void append_tag_to_input_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void output_Copy_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void ToggleVars_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void help_menu_Opening(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);
        void help_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void show_help(std::wstring_view tag);
        void help_link_variables_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_link_numbers_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_link_fp_and_integer_arithmetic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_link_fp_arithmetic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_link_integer_arithmetic_and_bitwise_logic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_link_scientific_functions_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_link_operator_precedence_and_associativity_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
        void help_link_quick_start_guide_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const& sender, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const& args);
    };
}

namespace winrt::tcalc::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
