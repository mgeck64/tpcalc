#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include <cassert>
#include <algorithm>

using namespace winrt;
using namespace Windows;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::ViewManagement;
using namespace Windows::System;

namespace winrt::tcalc::implementation
{
    MainPage::MainPage() : parser{[&]{on_vars_changed();}}
    {
        InitializeComponent();
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}

bool winrt::tcalc::implementation::MainPage::initializing_app = true;

inline auto winrt::tcalc::implementation::MainPage::make_size(double width, double height) -> Size {
    assert(width - static_cast<float>(width) < 1);
    assert(height - static_cast<float>(height) < 1);
    return Size(static_cast<float>(width), static_cast<float>(height));
}

void winrt::tcalc::implementation::MainPage::page_Loaded(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    default_page_size = make_size(calcPanel().ActualWidth(), calcPanel().ActualHeight() + bottomAppBar().ActualHeight());
    auto local_settings = Storage::ApplicationData::Current().LocalSettings();
    assert(varsPanel().Visibility() == Visibility::Collapsed);
    ApplicationView::GetForCurrentView().SetPreferredMinSize(default_page_size);
    if (!local_settings.Values().HasKey(L"launchedWithPrefSize")) {
        ApplicationView::PreferredLaunchViewSize(default_page_size);
        ApplicationView::PreferredLaunchWindowingMode(ApplicationViewWindowingMode::PreferredLaunchViewSize);
        local_settings.Values().Insert(L"launchedWithPrefSize", box_value(true)); // causes changes to have immediate effect

        // make sure help is visible in PC mode; in tablet mode this should be
        // redundant as help should be visible by default
        show_help(L"help_quick_start_guide");
    }

    initial_calcPanel_size = make_size(calcPanel().ActualWidth(), calcPanel().ActualHeight());
    initial_input_size = make_size(input().ActualWidth(), input().ActualHeight());
    initial_output_size = make_size(output().Width(), output().ActualHeight()); // output.ActualWidth() is returning 0 for some unknown reason
    size_values_valid = true;

    update_page_ui();
    on_vars_changed();

    { // Mode and Inp. Type menu check mark icon (selection indicator)
        auto symbol = Symbol::Accept; // check mark icon

        input_mode_fpd().Icon(SymbolIcon(symbol));
        input_mode_bin().Icon(SymbolIcon(symbol));
        input_mode_oct().Icon(SymbolIcon(symbol));
        input_mode_dec().Icon(SymbolIcon(symbol));
        input_mode_hex().Icon(SymbolIcon(symbol));

        output_mode_bin().Icon(SymbolIcon(symbol));
        output_mode_oct().Icon(SymbolIcon(symbol));
        output_mode_dec().Icon(SymbolIcon(symbol));
        output_mode_hex().Icon(SymbolIcon(symbol));

        integer_result_type_int8().Icon(SymbolIcon(symbol));
        integer_result_type_uint8().Icon(SymbolIcon(symbol));
        integer_result_type_int16().Icon(SymbolIcon(symbol));
        integer_result_type_uint16().Icon(SymbolIcon(symbol));
        integer_result_type_int32().Icon(SymbolIcon(symbol));
        integer_result_type_uint32().Icon(SymbolIcon(symbol));
        integer_result_type_int64().Icon(SymbolIcon(symbol));
        integer_result_type_uint64().Icon(SymbolIcon(symbol));

        update_mode_display();
    }

    set_output_to_last_val();
}

void winrt::tcalc::implementation::MainPage::page_SizeChanged(winrt::Windows::Foundation:: IInspectable const&, winrt::Windows::UI::Xaml::SizeChangedEventArgs const&)
{
    update_page_ui();
}

void winrt::tcalc::implementation::MainPage::set_output_to_last_val() {
    // if outputType() is hidden, show it
    if (outputType().Visibility() == Visibility::Collapsed) { // ...ActualWidth() is unreliable, so using ...Width()
        output().Width(output().Width() - outputType().Width());
        outputType().Visibility(Visibility::Visible);
    }

    std::wstring text;
    char_helper::append_to(text, parser_type::num_type_short_txt.at(parser.last_val().index()));
    text += L":";
    outputType().Text(text);

    std::wostringstream out_buf;
    out_buf << outputter(parser.last_val());
    auto out_str = out_buf.str();
    output().Text(out_str);
    outputted_last_val = true;
}

void winrt::tcalc::implementation::MainPage::set_output_to_text(std::wstring_view text) {
    // if outputType is showing, hide it
    if (outputType().Visibility() == Visibility::Visible) {
        outputType().Visibility(Visibility::Collapsed);
        output().Width(output().Width() + outputType().Width());
    }

    output().Text(text);
    outputted_last_val = false;
}

inline double winrt::tcalc::implementation::MainPage::space_for_XPanel() {
    double space_for_XPanel_ = ActualHeight() - calcPanel().ActualHeight() - bottomAppBar().ActualHeight() - 10; // - 10 for margin (can't figure how to do this in AXML)
    if (space_for_XPanel_ < 0)
        space_for_XPanel_ = 0;
    return space_for_XPanel_;
}

inline void winrt::tcalc::implementation::MainPage::update_XPanel_button_labels(double space_for_XPanel_) {
    // space_for_XPanel_ is optimization for update_page_ui to avoid duplicate call to space_for_XPanel()
    assert(space_for_XPanel_ == space_for_XPanel());
    bool show_help = space_for_XPanel_ == 0 || helpPanel().Visibility() == Visibility::Collapsed;
    help_menu_hide_help().IsEnabled(!show_help);
    
    bool show_vars = space_for_XPanel_ == 0 || varsPanel().Visibility() == Visibility::Collapsed;
    auto varsButtonLabel = show_vars ? L"Variables" : L"Hide Variables";
    if (varsButton().Label() != varsButtonLabel)
        varsButton().Label(varsButtonLabel);
} 

void winrt::tcalc::implementation::MainPage::update_page_ui() {
    if (size_values_valid) {
        auto delta_width = ActualWidth() - default_page_size.Width;
        if (initial_calcPanel_size.Width + delta_width >= 0)
            calcPanel().Width(initial_calcPanel_size.Width + delta_width);
        if (initial_input_size.Width + delta_width >= 0)
            input().Width(initial_input_size.Width + delta_width);
        auto output_padding = outputType().Visibility() == Visibility::Visible ? 0 : outputType().Width(); // ...ActualWidth is unreliable so using ...Width(), which is probably what we want anyhow
        if (initial_output_size.Width + delta_width + output_padding >= 0)
            output().Width(initial_output_size.Width + delta_width + output_padding);
    }

    auto space_for_XPanel_ = space_for_XPanel();
    if (varsPanel().ActualHeight() != space_for_XPanel_)
        varsPanel().Height(space_for_XPanel_);
    if (helpPanel().ActualHeight() != space_for_XPanel_)
        helpPanel().Height(space_for_XPanel_);

    update_XPanel_button_labels(space_for_XPanel_);
}

void winrt::tcalc::implementation::MainPage::update_mode_menu() {
    input_mode_fpd().Icon().Visibility(parser.default_radix() == radices::decimal ? Visibility::Visible : Visibility::Collapsed);
    input_mode_bin().Icon().Visibility(parser.default_radix() == radices::base2 ? Visibility::Visible : Visibility::Collapsed);
    input_mode_oct().Icon().Visibility(parser.default_radix() == radices::base8 ? Visibility::Visible : Visibility::Collapsed);
    input_mode_dec().Icon().Visibility(parser.default_radix() == radices::base10 ? Visibility::Visible : Visibility::Collapsed);
    input_mode_hex().Icon().Visibility(parser.default_radix() == radices::base16 ? Visibility::Visible : Visibility::Collapsed);

    output_mode_bin().Icon().Visibility(outputter.radix() == radices::base2 ? Visibility::Visible : Visibility::Collapsed);
    output_mode_oct().Icon().Visibility(outputter.radix() == radices::base8 ? Visibility::Visible : Visibility::Collapsed);
    output_mode_dec().Icon().Visibility((outputter.radix() == radices::decimal || outputter.radix() == radices::base10) ? Visibility::Visible : Visibility::Collapsed);
    output_mode_hex().Icon().Visibility(outputter.radix() == radices::base16 ? Visibility::Visible : Visibility::Collapsed);
}

void winrt::tcalc::implementation::MainPage::update_mode_display() {
    switch (parser.default_radix()) {
    case radices::decimal:
        inputModeText().Text(L"Input: Floating Point Decimal");
        break;
    case radices::base2:
        inputModeText().Text(L"Input: Integer Binary");
        break;
    case radices::base8:
        inputModeText().Text(L"Input: Integer Octal");
        break;
    case radices::base10:
        inputModeText().Text(L"Input: Integer/Flt. Pt. Decimal");
        break;
    case radices::base16:
        inputModeText().Text(L"Input: Integer/Flt. Pt. Hexadecimal");
        break;
    }

    switch (outputter.radix()) {
    case radices::base2:
        outputModeText().Text(L"Output: Binary");
        break;
    case radices::base8:
        outputModeText().Text(L"Output: Octal");
        break;
    case radices::decimal:
    case radices::base10:
        outputModeText().Text(L"Output: Decimal");
        break;
    case radices::base16:
        outputModeText().Text(L"Output: Hexadecimal");
        break;
    }
}

void winrt::tcalc::implementation::MainPage::update_integer_result_type_menu() {
    integer_result_type_int8().Icon().Visibility(parser.int_result_tag() == parser_type::int8_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_uint8().Icon().Visibility(parser.int_result_tag() == parser_type::uint8_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_int16().Icon().Visibility(parser.int_result_tag() == parser_type::int16_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_uint16().Icon().Visibility(parser.int_result_tag() == parser_type::uint16_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_int32().Icon().Visibility(parser.int_result_tag() == parser_type::int32_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_uint32().Icon().Visibility(parser.int_result_tag() == parser_type::uint32_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_int64().Icon().Visibility(parser.int_result_tag() == parser_type::int64_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_uint64().Icon().Visibility(parser.int_result_tag() == parser_type::uint64_tag ? Visibility::Visible : Visibility::Collapsed);
}

void winrt::tcalc::implementation::MainPage::mode_menu_Opening(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&) {
    update_mode_menu();
}

void winrt::tcalc::implementation::MainPage::input_mode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto tag = unbox_value<hstring>(sender.as<FrameworkElement>().Tag());
    if (tag == L"fpd")
        parser.default_radix(radices::decimal);
    else if (tag == L"bin")
        parser.default_radix(radices::base2);
    else if (tag == L"oct")
        parser.default_radix(radices::base8);
    else if (tag == L"dec")
        parser.default_radix(radices::base10);
    else if (tag == L"hex")
        parser.default_radix(radices::base16);
    else
        assert(false); // missed one
    input().Focus(FocusState::Programmatic);
    update_mode_display();
}

void winrt::tcalc::implementation::MainPage::output_mode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto old_radix = outputter.radix();
    auto tag = unbox_value<hstring>(sender.as<FrameworkElement>().Tag());
    if (tag == L"bin")
        outputter.radix(radices::base2);
    else if (tag == L"oct")
        outputter.radix(radices::base8);
    else if (tag == L"dec")
        outputter.radix(radices::base10);
    else if (tag == L"hex")
        outputter.radix(radices::base16);
    else
        assert(false); // missed one
    if (outputted_last_val)
        set_output_to_last_val(); // will re-display last value in selected mode
    if (old_radix != outputter.radix())
        on_vars_changed(); // re-output in new radix
    input().Focus(FocusState::Programmatic);
    update_mode_display();
}

void winrt::tcalc::implementation::MainPage::integer_result_type_menu_Opening(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&) {
    update_integer_result_type_menu();
}

void winrt::tcalc::implementation::MainPage::integer_result_type_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto tag = unbox_value<hstring>(sender.as<FrameworkElement>().Tag());
    if (tag == L"int8")
        parser.int_result_tag(parser_type::int8_tag);
    else if (tag == L"uint8")
        parser.int_result_tag(parser_type::uint8_tag);
    else if (tag == L"int16")
        parser.int_result_tag(parser_type::int16_tag);
    else if (tag == L"uint16")
        parser.int_result_tag(parser_type::uint16_tag);
    else if (tag == L"int32")
        parser.int_result_tag(parser_type::int32_tag);
    else if (tag == L"uint32")
        parser.int_result_tag(parser_type::uint32_tag);
    else if (tag == L"int64")
        parser.int_result_tag(parser_type::int64_tag);
    else if (tag == L"uint64")
        parser.int_result_tag(parser_type::uint64_tag);
    else
        assert(false); // missed one
    if (outputted_last_val)
        set_output_to_last_val(); // re-display last value casted to the selected type
    input().Focus(FocusState::Programmatic);
}

void winrt::tcalc::implementation::MainPage::eval_input()
{
    auto input_str = input().Text();
    bool input_is_blank = false;
    try {
        input_is_blank = !parser.eval(input_str.c_str());
        set_output_to_last_val();
        input().Text(L""); // clear for next input (even if input_is_blank is true, input may have whitespace chars)
    } catch (const parser_type::parse_error& e) {
        e.assert_view_is_valid_for(input_str.c_str());
        set_output_to_text(e.error_str());
        input().Select(e.tok.tok_str.data() - input_str.c_str(), e.tok.tok_str.size());
    }

    if (!input_is_blank) { // save input string so can be recalled later
        last_input = input_str;
        assert(last_inputs.size() <= max_last_inputs_size);
        if (last_inputs.size() < max_last_inputs_size)
            last_inputs.emplace_back(input_str);
        else {
            assert(last_inputs.begin() < last_inputs.end());
            std::move(last_inputs.begin() + 1, last_inputs.end(), last_inputs.begin());
            last_inputs.back() = input_str;
        }
        last_inputs_idx = last_inputs.size(); // equivalent to end()
    }
}

void winrt::tcalc::implementation::MainPage::page_KeyDown(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& /*e*/)
{
}

void winrt::tcalc::implementation::MainPage::input_KeyDown(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e)
{
    if (e.Key() == VirtualKey::Enter) {
        eval_input();
    } else if (e.Key() == VirtualKey::F3) { // recall last input
        input().SelectedText(last_input); // should insert new_text at caret position if nothing was selected
        input().Select(input().SelectionStart() + last_input.size(), 0);
    } else if (e.Key() == VirtualKey::Up) {
        if (last_inputs_idx > 0) {
            --last_inputs_idx;
            auto& text = last_inputs[last_inputs_idx];
            input().Text(text);
            input().Select(text.size(), 0); // place caret at end
        }
    } else if (e.Key() == VirtualKey::Down) {
        if (last_inputs_idx + 1 < last_inputs.size()) {
            ++last_inputs_idx;
            auto& text = last_inputs[last_inputs_idx];
            input().Text(text);
            input().Select(text.size(), 0); // place caret at end
        }
    }
}

void winrt::tcalc::implementation::MainPage::append_tag_to_input_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto new_text = unbox_value<hstring>(sender.as<FrameworkElement>().Tag());
    input().SelectedText(new_text); // should insert new_text at caret position if nothing was selected
    input().Select(input().SelectionStart() + new_text.size(), 0);
    input().Focus(FocusState::Programmatic);
}

inline void winrt::tcalc::implementation::MainPage::TryResizeView(Size size) {
    // make sure size is no smaller than min. page size (default_page_size) else
    // calling TryResizeView(size) will do nothing because of min. size setting.
    // size can become smaller than default_page_size due to rounding/tolerance
    // error
    if (size.Width < default_page_size.Width)
        size.Width = default_page_size.Width;
    if (size.Height < default_page_size.Height)
        size.Height = default_page_size.Height;
    ApplicationView::GetForCurrentView().TryResizeView(size);
}

void winrt::tcalc::implementation::MainPage::output_Copy_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto start = output().SelectionStart();
    auto end = output().SelectionEnd();
    if (!output().SelectedText().size()) // note: start == end doesn't work
        output().SelectAll();
    output().CopySelectionToClipboard();
    output().Select(start, end); // undo SelectAll
}

void winrt::tcalc::implementation::MainPage::on_vars_changed() {
    if (!parser.vars().size()) {
        varsTextBlock().Text(L"There are no variables to show.");
        return;
    }

    std::wstringstream out_buf;
    auto vars_begin = parser.vars().begin();
    auto vars_end = parser.vars().end();
    for (auto var_pos = vars_begin; var_pos != vars_end; ++var_pos) {
        if (var_pos != vars_begin)
            out_buf << '\n';
        out_buf << var_pos->first << " = "
            << parser_type::num_type_short_txt.at(var_pos->second.val_var.index())
            << ": " << outputter(var_pos->second.val_var);
    }
    varsTextBlock().Text(out_buf.str());
}

void winrt::tcalc::implementation::MainPage::ToggleVars_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    if (ActualHeight() <= default_page_size.Height // should be helpPanel.ActualHeight() == 0 but for unknown reason helpPanel.ActualHeight() is not reliable
            || varsPanel().Visibility() == Visibility::Collapsed) {
        varsPanel().Visibility(Visibility::Visible);
        helpPanel().Visibility(Visibility::Collapsed);
        TryResizeView(make_size(default_page_size.Width, 300));
    } else if (varsPanel().Visibility() == Visibility::Visible) {
        varsPanel().Visibility(Visibility::Collapsed);
        helpPanel().Visibility(Visibility::Collapsed);
        TryResizeView(default_page_size);
    } else
        assert(false); // missed something
    input().Focus(FocusState::Programmatic);
    update_XPanel_button_labels();
}

void winrt::tcalc::implementation::MainPage::help_menu_Opening(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&) {
    update_XPanel_button_labels();
}

void winrt::tcalc::implementation::MainPage::help_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    show_help(unbox_value<hstring>(sender.as<FrameworkElement>().Tag()));
}

void winrt::tcalc::implementation::MainPage::show_help(std::wstring_view tag) {
    ScrollViewer visible;
    bool have_visible = false;
    if (help_quick_start_guide().Visibility() == Visibility::Visible) {
        visible = help_quick_start_guide();
        have_visible = true;
    } else if (help_variables().Visibility() == Visibility::Visible) {
        visible = help_variables();
        have_visible = true;
    } else if (help_numbers().Visibility() == Visibility::Visible) {
        visible = help_numbers();
        have_visible = true;
    } else if (help_fp_and_integer_arithmetic_operators().Visibility() == Visibility::Visible) {
        visible = help_fp_and_integer_arithmetic_operators();
        have_visible = true;
    } else if (help_fp_arithmetic_operators().Visibility() == Visibility::Visible) {
        visible = help_fp_arithmetic_operators();
        have_visible = true;
    } else if (help_integer_arithmetic_and_bitwise_logic_operators().Visibility() == Visibility::Visible) {
        visible = help_integer_arithmetic_and_bitwise_logic_operators();
        have_visible = true;
    } else if (help_scientific_functions().Visibility() == Visibility::Visible) {
        visible = help_scientific_functions();
        have_visible = true;
    } else if (help_operator_precedence_and_associativity().Visibility() == Visibility::Visible) {
        visible = help_operator_precedence_and_associativity();
        have_visible = true;
    }

    ScrollViewer make_visible;
    bool making_visible = false;
    if (tag == L"help_quick_start_guide") {
        make_visible = help_quick_start_guide();
        making_visible = true;
    } else if (tag == L"help_variables") {
        make_visible = help_variables();
        making_visible = true;
    } else if (tag == L"help_numbers") {
        make_visible = help_numbers();
        making_visible = true;
    } else if (tag == L"help_fp_and_integer_arithmetic_operators") {
        make_visible = help_fp_and_integer_arithmetic_operators();
        making_visible = true;
    } else if (tag == L"help_fp_arithmetic_operators") {
        make_visible = help_fp_arithmetic_operators();
        making_visible = true;
    } else if (tag == L"help_integer_arithmetic_and_bitwise_logic_operators") {
        make_visible = help_integer_arithmetic_and_bitwise_logic_operators();
        making_visible = true;
    } else if (tag == L"help_scientific_functions") {
        make_visible = help_scientific_functions();
        making_visible = true;
    } else if (tag == L"help_operator_precedence_and_associativity") {
        make_visible = help_operator_precedence_and_associativity();
        making_visible = true;
    } else
        assert(tag == L"help_hide_help");

    if (have_visible)
        visible.Visibility(Visibility::Collapsed);
    if (making_visible) {
        varsPanel().Visibility(Visibility::Collapsed); // incase showing

        helpPanel().Visibility(Visibility::Visible);
        make_visible.Visibility(Visibility::Visible);
        const auto& info = Graphics::Display::DisplayInformation::GetForCurrentView();
        auto height = info.ScreenHeightInRawPixels();
        auto scale_factor = info.RawPixelsPerViewPixel();
        TryResizeView(make_size(500, (height / scale_factor) * 0.8));
    } else if (varsPanel().Visibility() == Visibility::Collapsed) {
        helpPanel().Visibility(Visibility::Collapsed);
        TryResizeView(default_page_size);
    }
}

void winrt::tcalc::implementation::MainPage::help_link_variables_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_variables");}

void winrt::tcalc::implementation::MainPage::help_link_numbers_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_numbers");}

void winrt::tcalc::implementation::MainPage::help_link_fp_and_integer_arithmetic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_fp_and_integer_arithmetic_operators");}

void winrt::tcalc::implementation::MainPage::help_link_fp_arithmetic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_fp_arithmetic_operators");}

void winrt::tcalc::implementation::MainPage::help_link_integer_arithmetic_and_bitwise_logic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_integer_arithmetic_and_bitwise_logic_operators");}

void winrt::tcalc::implementation::MainPage::help_link_scientific_functions_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_scientific_functions");}

void winrt::tcalc::implementation::MainPage::help_link_operator_precedence_and_associativity_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_operator_precedence_and_associativity");}

void winrt::tcalc::implementation::MainPage::help_link_quick_start_guide_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_quick_start_guide");}
