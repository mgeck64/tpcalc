#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include <cassert>
#include <algorithm>
#include "..\rdcalc\calc_util.h"

using namespace winrt;
using namespace Windows;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::ViewManagement;
using namespace Windows::System;
using namespace ::tpcalc;

namespace winrt::tpcalc::implementation
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

void winrt::tpcalc::implementation::MainPage::page_Loaded(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    assert(varsPanel().Visibility() == Visibility::Collapsed);
    assert(helpPanel().Visibility() == Visibility::Collapsed);

    auto local_settings = Storage::ApplicationData::Current().LocalSettings();

    default_page_size = DSize(calcPanel().ActualWidth(), calcPanel().ActualHeight() + bottomAppBar().ActualHeight());
    XPanel_hint_for_vars.Width = default_page_size.Width;
    initial_calcPanel_size = DSize(calcPanel().ActualWidth(), calcPanel().ActualHeight());
    initial_input_size = DSize(input().Width(), input().Height());
    initial_output_size = DSize(output().Width(), output().Height()); // output.ActualWidth() is returning 0 for some unknown reason
    input_height_limit_when_XPanel_showing
        = unbox_value_or(local_settings.Values().Lookup(L"input_height_limit_when_XPanel_showing"), initial_input_size.DHeight());
    output_height_limit_when_XPanel_showing
        = unbox_value_or(local_settings.Values().Lookup(L"output_height_limit_when_XPanel_showing"), initial_output_size.DHeight());
    size_values_valid = true;

    auto input_size = unbox_value_or(local_settings.Values().Lookup(L"input_size"), initial_input_size);
    auto output_size = unbox_value_or(local_settings.Values().Lookup(L"output_size"), initial_output_size);
    input().Width(input_size.Width);
    input().Height(input_size.Height);
    output().Width(output_size.Width);
    output().Height(output_size.Height);
    outputType().Height(output().Height());

    ToolTipService::SetToolTip(output(), outputTip);

    ApplicationView::GetForCurrentView().SetPreferredMinSize(default_page_size);
    if (!local_settings.Values().HasKey(L"Launched")) {
        local_settings.Values().Insert(L"Launched", box_value(true));
        show_help(L"help_quick_start_guide_basic", true /*first_app_launch*/);
     } else {
        auto XPanel = unbox_value_or(local_settings.Values().Lookup(L"XPanel"), L"none");
        if (XPanel == L"vars")
            show_vars(L"vars_show");
        else if (static_cast<std::wstring_view>(XPanel).substr(0, 5) == L"help_")
            show_help(XPanel);
        else if (XPanel == L"none")
            update_page_layout();
        else {
            assert(false); // missed one
            update_page_layout();
        }
    }

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

void winrt::tpcalc::implementation::MainPage::page_SizeChanged(winrt::Windows::Foundation:: IInspectable const&, winrt::Windows::UI::Xaml::SizeChangedEventArgs const&)
// indirectly called by TryResizeView.
// also called in response to the user resizing the page window
{
    if (size_values_valid) {
        auto extra_height =
            ActualHeight()
            - calcPanel().ActualHeight()
            - bottomAppBar().ActualHeight();

        assert(input().Height());
        auto p = (input().Height() + output().Height()) / input().Height();
        assert(p);
        auto extra_input_height = extra_height / p;

        if (extra_height && XPanel().Visibility() == Visibility::Collapsed) {
            input().Height(input().Height() + extra_input_height);
            output().Height(output().Height() + (extra_height - extra_input_height));
            outputType().Height(output().Height());

            auto input_size = DSize(input().Width(), input().Height());
            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"input_size", box_value(input_size));
            auto output_size = DSize(output().Width(), output().Height());
            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"output_size", box_value(output_size));

            input_height_limit_when_XPanel_showing = input().Height();
            output_height_limit_when_XPanel_showing = output().Height();

            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"input_height_limit_when_XPanel_showing", box_value(input_height_limit_when_XPanel_showing));
            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"output_height_limit_when_XPanel_showing", box_value(output_height_limit_when_XPanel_showing));
        } else if (extra_height
                && (extra_height + input().Height() + output().Height()
                    <= input_height_limit_when_XPanel_showing + output_height_limit_when_XPanel_showing)) {
            input().Height(input().Height() + extra_input_height);
            output().Height(output().Height() + (extra_height - extra_input_height));
            outputType().Height(output().Height());

            auto input_size = DSize(input().Width(), input().Height());
            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"input_size", box_value(input_size));
            auto output_size = DSize(output().Width(), output().Height());
            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"output_size", box_value(output_size));
        }

    }

    update_page_layout();
}

void winrt::tpcalc::implementation::MainPage::set_output_to_last_val() {
    // if outputType() is hidden, show it
    if (outputType().Visibility() == Visibility::Collapsed) { // ...ActualWidth() is unreliable, so using ...Width()
        output().Width(output().Width() - outputType().Width());
        outputType().Visibility(Visibility::Visible);
    }

    std::wstring text;
    char_helper::append_to(text, parser_val_type_short_txt.at(parser.last_val().index()));
    text += L":";
    outputType().Text(text);

    std::wostringstream out_stream;
    out_stream << outputter(parser.last_val());
    output().Text(out_stream.str());

    auto tip_outputter = outputter_type(radices::decimal);
    out_stream = std::wostringstream();
    out_stream << "Decimal: " << tip_outputter(parser.last_val());
    outputTip.Content(box_value(out_stream.str()));

    outputted_last_val = true;
}

void winrt::tpcalc::implementation::MainPage::set_output_to(std::wstring_view text) {
    // if outputType is showing, hide it
    if (outputType().Visibility() == Visibility::Visible) {
        outputType().Visibility(Visibility::Collapsed);
        output().Width(output().Width() + outputType().Width());
    }

    output().Text(text);
    outputTip.Content(box_value(text));
    outputted_last_val = false;
}

inline void winrt::tpcalc::implementation::MainPage::update_button_labels() {
    bool show_help = helpPanel().Visibility() == Visibility::Collapsed;
    help_menu_hide_help().IsEnabled(!show_help);

    bool show_vars = varsPanel().Visibility() == Visibility::Collapsed;
    auto varsButton_label = show_vars ? L"Variables" : L"Hide Variables";
    auto varsButton_tag = show_vars ? L"vars_show" : L"vars_hide";
    if (varsButton().Label() != varsButton_label)
        varsButton().Label(varsButton_label);
    if (unbox_value_or(varsButton().Tag(), L"") != varsButton_tag)
        varsButton().Tag(box_value(varsButton_tag));

    { // min_in_out_Button()
        auto tag = unbox_value_or(min_in_out_Button().Tag(), L"");
        if (input().Height() < initial_input_size.DHeight() + 2) { // + 2 for rounding/tolerance error (approx. value; empirically determined)
            min_in_out_Button().Tag(box_value(L"min_in_out_down"));
            min_in_out_Button().Content(box_value(L"▼"));
            min_in_out_Button().IsEnabled(true);
        } else {
            min_in_out_Button().Tag(box_value(L"min_in_out_up"));
            min_in_out_Button().Content(box_value(L"▲"));
            min_in_out_Button().IsEnabled(true);
        }
    }
}

void winrt::tpcalc::implementation::MainPage::update_page_layout() {
    if (size_values_valid) {
        if (input().Height() < initial_input_size.DHeight() + 2) // + 2 for rounding/tolerance error (approx. value; empirically determined)
            input().TextWrapping(TextWrapping::NoWrap);
        else
            input().TextWrapping(TextWrapping::Wrap);

        auto height_space_for_XPanel = 
            ActualHeight()

            // - calcPanel().ActualHeight() // SIGH! ActualHeight is unreliable here!; need to calculate what this should be:
            - initial_calcPanel_size.DHeight()
            + initial_input_size.DHeight()
            + initial_output_size.DHeight()
            - input().Height()
            - output().Height()

            - XPanel_margin_bottom
            - bottomAppBar().ActualHeight();
        if (height_space_for_XPanel < 0)
            height_space_for_XPanel = 0;

        if (varsPanel().Visibility() == Visibility::Visible)
            varsPanel().Height(height_space_for_XPanel);
        else if (helpPanel().Visibility() == Visibility::Visible)
            helpPanel().Height(height_space_for_XPanel);

        auto delta_width = ActualWidth() - default_page_size.DWidth();
        if (initial_calcPanel_size.DWidth() + delta_width >= 0)
            calcPanel().Width(initial_calcPanel_size.DWidth() + delta_width);
        if (initial_input_size.DWidth() + delta_width >= 0)
            input().Width(initial_input_size.DWidth() + delta_width);

        auto output_width_padding = outputType().Visibility() == Visibility::Visible ? 0 : outputType().Width();
        if (initial_output_size.DWidth() + delta_width + output_width_padding >= 0)
            output().Width(initial_output_size.DWidth() + delta_width + output_width_padding);
    }

    update_button_labels();
}

void winrt::tpcalc::implementation::MainPage::update_mode_menu() {
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

void winrt::tpcalc::implementation::MainPage::update_mode_display() {
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

void winrt::tpcalc::implementation::MainPage::update_integer_result_type_menu() {
    integer_result_type_int8().Icon().Visibility(parser.int_result_tag() == parser_type::int8_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_uint8().Icon().Visibility(parser.int_result_tag() == parser_type::uint8_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_int16().Icon().Visibility(parser.int_result_tag() == parser_type::int16_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_uint16().Icon().Visibility(parser.int_result_tag() == parser_type::uint16_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_int32().Icon().Visibility(parser.int_result_tag() == parser_type::int32_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_uint32().Icon().Visibility(parser.int_result_tag() == parser_type::uint32_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_int64().Icon().Visibility(parser.int_result_tag() == parser_type::int64_tag ? Visibility::Visible : Visibility::Collapsed);
    integer_result_type_uint64().Icon().Visibility(parser.int_result_tag() == parser_type::uint64_tag ? Visibility::Visible : Visibility::Collapsed);
}

void winrt::tpcalc::implementation::MainPage::mode_menu_Opening(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&) {
    update_mode_menu();
}

void winrt::tpcalc::implementation::MainPage::input_mode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto tag = unbox_value_or(sender.as<FrameworkElement>().Tag(), L"");
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

void winrt::tpcalc::implementation::MainPage::output_mode_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto old_radix = outputter.radix();
    auto tag = unbox_value_or(sender.as<FrameworkElement>().Tag(), L"");
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

void winrt::tpcalc::implementation::MainPage::integer_result_type_menu_Opening(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&) {
    update_integer_result_type_menu();
}

void winrt::tpcalc::implementation::MainPage::integer_result_type_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto tag = unbox_value_or(sender.as<FrameworkElement>().Tag(), L"");
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

void winrt::tpcalc::implementation::MainPage::evaluate_input()
{
    auto input_str = input().Text();
    try {
        auto input_evaluated = parser.evaluate(input_str.c_str());
        set_output_to_last_val();
        input().Text(L""); // clear for next input (input may have whitespace chars even if "blank")

        if (input_evaluated) { // save input string so can be recalled later
            last_input = input_str;
            assert(last_inputs.size() <= max_last_inputs_size);
            if (last_inputs.size() < max_last_inputs_size)
                last_inputs.emplace_back(input_str);
            else {
                assert(last_inputs.begin() < last_inputs.end());
                std::move(last_inputs.begin() + 1, last_inputs.end(), last_inputs.begin());
                last_inputs.back() = input_str;
            }
            last_inputs_idx = last_inputs.size();
        }
    } catch (const parse_error& e) {
        set_output_to(e.error_str());
        if (e.view_is_valid_for(input_str.c_str()))
            input().Select(e.tok.tok_str.data() - input_str.c_str(), e.tok.tok_str.size());
    } catch (const internal_error& e) {
        std::wostringstream out_stream;
        out_stream << "Unexpected error in " << e.str.c_str() << '.';
        set_output_to(out_stream.str());
    }
}

void winrt::tpcalc::implementation::MainPage::input_KeyDown(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e)
{
    using namespace winrt::Windows::UI::Core;
    auto it = CoreWindow::GetForCurrentThread();
    enum {shift_down_flag = 1, ctrl_down_flag = 2, alt_down_flag = 4}; // bit flags
    auto shift_down = ((it.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down) ? shift_down_flag : 0;
    auto ctrl_down = ((it.GetKeyState(VirtualKey::Control) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down) ? ctrl_down_flag : 0;
    auto alt_down = ((it.GetKeyState(VirtualKey::Menu) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down) ? alt_down_flag : 0;
    auto key_modifiers = shift_down | ctrl_down | alt_down;

    if (e.Key() == VirtualKey::Enter && !key_modifiers)
        evaluate_input();
    else if ((e.Key() == VirtualKey::F3) && !key_modifiers) { // recall last input
        input().SelectedText(last_input); // replace selected text or insert text at caret position if nothing was selected
        input().Select(input().SelectionStart() + input().SelectionLength(), 0); // place caret at end of new text
    } else if ((e.Key() == VirtualKey::Up) && (key_modifiers == alt_down_flag)) { // recall prior input
        if (last_inputs_idx > 0) {
            --last_inputs_idx;
            auto& text = last_inputs[last_inputs_idx];
            input().Text(text); // replacing selection as for F3 is problematic here when there's more than one input; just replace entire input
            input().Select(std::numeric_limits<int32_t>::max(), 0); // place caret at end of input
        }
    } else if ((e.Key() == VirtualKey::Down) && (key_modifiers == alt_down_flag)) { // recall prior input
        if (last_inputs_idx + 1 < last_inputs.size()) {
            ++last_inputs_idx;
            auto& text = last_inputs[last_inputs_idx];
            input().Text(text); // see comment above
            input().Select(std::numeric_limits<int32_t>::max(), 0); // see comment above
        }
    }
}

void winrt::tpcalc::implementation::MainPage::append_tag_to_input_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto new_text = unbox_value_or(sender.as<FrameworkElement>().Tag(), L"");
    input().SelectedText(new_text); // should insert new_text at caret position if nothing was selected
    input().Select(input().SelectionStart() + new_text.size(), 0);
    input().Focus(FocusState::Programmatic);
}

inline void winrt::tpcalc::implementation::MainPage::TryResizeView(DSize size) {
    // make sure size is no smaller than min. page size (default_page_size) else
    // calling windows TryResizeView(size) will do nothing because of min. size
    // setting. note: size can become smaller than default_page_size due to
    // rounding/tolerance error
    if (size.Width < default_page_size.Width)
        size.Width = default_page_size.Width;
    if (size.Height < default_page_size.Height)
        size.Height = default_page_size.Height;

    ApplicationView::GetForCurrentView().TryResizeView(size);
    // we will call update_page_layout again incase TryResizeView doesn't
    // trigger page_SizeChanged. this means that in normal cases
    // update_page_layout will be called twice unnecessarily. notes: return
    // value of TryResizeView is unreliable; also, TryResizeView seems to
    // operate asynchronously so we can't use a flag to suppress
    // update_page_layout there because update_page_layout would be called here
    // at the wrong time. we don't just call page_SizeChange directly here
    // because TryResizeView also causes new size to be stored so window size
    // can be restored when app is relaunched
    update_page_layout();
}

void winrt::tpcalc::implementation::MainPage::output_Copy_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto start = output().SelectionStart();
    auto end = output().SelectionEnd();
    if (!output().SelectedText().size()) // note: start == end doesn't work
        output().SelectAll();
    output().CopySelectionToClipboard();
    output().Select(start, end); // undo SelectAll
}

void winrt::tpcalc::implementation::MainPage::on_vars_changed() {
    if (!parser.vars().size()) {
        varsTextBlock().Text(L"There are no variables to show.");
        return;
    }

    std::wstringstream out_stream;
    for (auto var_pos = parser.vars().begin(); var_pos != parser.vars().end(); ++var_pos) {
        if (var_pos != parser.vars().begin())
            out_stream << '\n';
        out_stream << var_pos->first << " = "
            << parser_val_type_short_txt.at(var_pos->second.val_var.index())
            << ": " << outputter(var_pos->second.val_var);
    }
    varsTextBlock().Text(out_stream.str());
}

void winrt::tpcalc::implementation::MainPage::show_vars(std::wstring_view tag) {
    if (tag == L"vars_show") {
        varsPanel().Visibility(Visibility::Visible);
        helpPanel().Visibility(Visibility::Collapsed);
        XPanel().Visibility(Visibility::Visible);
        TryResizeView(width_and_height(XPanel_hint_for_vars));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"XPanel", box_value(L"vars"));
    } else if (tag == L"vars_hide") {
        auto XPanel_height = XPanel().ActualHeight() + XPanel_margin_bottom;
        varsPanel().Visibility(Visibility::Collapsed);
        helpPanel().Visibility(Visibility::Collapsed);
        XPanel().Visibility(Visibility::Collapsed);
        TryResizeView(default_page_size.DWidth(), ActualHeight() - XPanel_height);
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"XPanel", box_value(L"none"));
    } else
        assert(false); // missed something

    input().Focus(FocusState::Programmatic);
}

void winrt::tpcalc::implementation::MainPage::vars_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    show_vars(unbox_value_or(sender.as<FrameworkElement>().Tag(), L""));
}

void winrt::tpcalc::implementation::MainPage::vars_hide_vars_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{show_vars(L"vars_hide");}

void winrt::tpcalc::implementation::MainPage::help_menu_Opening(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&)
{
    update_button_labels();
}

void winrt::tpcalc::implementation::MainPage::help_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    show_help(unbox_value_or(sender.as<FrameworkElement>().Tag(), L""));
}

void winrt::tpcalc::implementation::MainPage::show_help(std::wstring_view tag, bool first_app_launch) {
    ScrollViewer visible;
    bool have_visible = false;
    if (help_quick_start_guide_basic().Visibility() == Visibility::Visible) {
        visible = help_quick_start_guide_basic();
        have_visible = true;
    } else if (help_quick_start_guide_advanced().Visibility() == Visibility::Visible) {
        visible = help_quick_start_guide_advanced();
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
    } else if (help_statistical_functions().Visibility() == Visibility::Visible) {
        visible = help_statistical_functions();
        have_visible = true;
    } else if (help_vector_arithmetic().Visibility() == Visibility::Visible) {
        visible = help_vector_arithmetic();
        have_visible = true;
    } else if (help_operator_precedence_and_associativity().Visibility() == Visibility::Visible) {
        visible = help_operator_precedence_and_associativity();
        have_visible = true;
    }

    ScrollViewer make_visible;
    bool making_visible = false;
    std::wstring_view help_title;
    if (tag == L"help_quick_start_guide_basic") {
        make_visible = help_quick_start_guide_basic();
        help_title = L"Quick Start Guide - Basic";
        making_visible = true;
    } else if (tag == L"help_quick_start_guide_advanced") {
        make_visible = help_quick_start_guide_advanced();
        help_title = L"Quick Start Guide - Advanced";
        making_visible = true;
    } else if (tag == L"help_variables") {
        make_visible = help_variables();
        help_title = L"Variables";
        making_visible = true;
    } else if (tag == L"help_numbers") {
        make_visible = help_numbers();
        help_title = L"Numbers";
        making_visible = true;
    } else if (tag == L"help_fp_and_integer_arithmetic_operators") {
        make_visible = help_fp_and_integer_arithmetic_operators();
        help_title = L"Floating Point and Integer Arithmetic Operators";
        making_visible = true;
    } else if (tag == L"help_fp_arithmetic_operators") {
        make_visible = help_fp_arithmetic_operators();
        help_title = L"Floating Poing Arithmetic Operators";
        making_visible = true;
    } else if (tag == L"help_integer_arithmetic_and_bitwise_logic_operators") {
        make_visible = help_integer_arithmetic_and_bitwise_logic_operators();
        help_title = L"Integer Arithmetic and Bitwise Logic Operators";
        making_visible = true;
    } else if (tag == L"help_scientific_functions") {
        make_visible = help_scientific_functions();
        help_title = L"Scientific Functions";
        making_visible = true;
    } else if (tag == L"help_statistical_functions") {
        make_visible = help_statistical_functions();
        help_title = L"Statistical Functions";
        making_visible = true;
    } else if (tag == L"help_vector_arithmetic") {
        make_visible = help_vector_arithmetic();
        help_title = L"Vector Arithmetic";
        making_visible = true;
    } else if (tag == L"help_operator_precedence_and_associativity") {
        make_visible = help_operator_precedence_and_associativity();
        help_title = L"Operator Precedence and Associativity";
        making_visible = true;
    } else
        assert(tag == L"help_hide_help");

    if (have_visible)
        visible.Visibility(Visibility::Collapsed);
    if (making_visible) {
        varsPanel().Visibility(Visibility::Collapsed); // incase showing
        helpPanel().Visibility(Visibility::Visible);
        make_visible.Visibility(Visibility::Visible);
        helpTitle().Text(help_title);
        XPanel().Visibility(Visibility::Visible);
        TryResizeView(width_and_height(XPanel_hint_for_help, first_app_launch));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"XPanel", box_value(tag));
    } else if (varsPanel().Visibility() == Visibility::Collapsed) {
        auto XPanel_height = XPanel().ActualHeight() + XPanel_margin_bottom;
        helpPanel().Visibility(Visibility::Collapsed);
        XPanel().Visibility(Visibility::Collapsed);
        TryResizeView(default_page_size.DWidth(), ActualHeight() - XPanel_height);
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"XPanel", box_value(L"none"));
    }

    input().Focus(FocusState::Programmatic);
}

void winrt::tpcalc::implementation::MainPage::help_quick_start_guide_basic_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_quick_start_guide_basic");}

void winrt::tpcalc::implementation::MainPage::help_quick_start_guide_advanced_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_quick_start_guide_advanced");}

void winrt::tpcalc::implementation::MainPage::help_variables_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_variables");}

void winrt::tpcalc::implementation::MainPage::help_numbers_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_numbers");}

void winrt::tpcalc::implementation::MainPage::help_fp_and_integer_arithmetic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_fp_and_integer_arithmetic_operators");}

void winrt::tpcalc::implementation::MainPage::help_fp_arithmetic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_fp_arithmetic_operators");}

void winrt::tpcalc::implementation::MainPage::help_integer_arithmetic_and_bitwise_logic_operators_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_integer_arithmetic_and_bitwise_logic_operators");}

void winrt::tpcalc::implementation::MainPage::help_scientific_functions_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_scientific_functions");}

void winrt::tpcalc::implementation::MainPage::help_statistical_functions_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_statistical_functions");}

void winrt::tpcalc::implementation::MainPage::help_vector_arithmetic_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_vector_arithmetic");}

void winrt::tpcalc::implementation::MainPage::help_operator_precedence_and_associativity_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_operator_precedence_and_associativity");}

void winrt::tpcalc::implementation::MainPage::help_hide_help_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{show_help(L"help_hide_help");}

void winrt::tpcalc::implementation::MainPage::min_in_out_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    input_height_limit_when_XPanel_showing = initial_input_size.DHeight();
    output_height_limit_when_XPanel_showing = initial_output_size.DHeight();

    auto delta_input_height = 0.0;
    auto delta_output_height = 0.0;
    auto resize_to_width = 0.0;
    bool handled = false;
    auto tag = unbox_value_or(sender.as<FrameworkElement>().Tag(), L"");
    if (tag == L"min_in_out_up") {
        delta_input_height = initial_input_size.DHeight() - input().Height();
        delta_output_height = initial_output_size.DHeight() - output().Height();
        if (XPanel().Visibility() == Visibility::Collapsed)
            resize_to_width = default_page_size.DWidth();
        else
            resize_to_width = ActualWidth();
        handled = true;
    } else if (tag == L"min_in_out_down") {
        input_height_limit_when_XPanel_showing *= min_in_out_down_scale_factor;
        output_height_limit_when_XPanel_showing *= min_in_out_down_scale_factor; // factor must be same as for input_height_limit_when_XPanel_showing to maintain proportionality of input and output else resizing view to minimum won't be quite right
        delta_input_height = input_height_limit_when_XPanel_showing - input().Height();
        delta_output_height = output_height_limit_when_XPanel_showing - output().Height();
        resize_to_width = ActualWidth();
        handled = true;
    } else if (tag != L"none")
        assert(false); // missed one

    if (handled) {
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"input_height_limit_when_XPanel_showing", box_value(input_height_limit_when_XPanel_showing));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"output_height_limit_when_XPanel_showing", box_value(output_height_limit_when_XPanel_showing));

        input().Height(input_height_limit_when_XPanel_showing);
        output().Height(output_height_limit_when_XPanel_showing);
        outputType().Height(output().Height());

        auto input_size = DSize(input().Width(), input().Height());
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"input_size", box_value(input_size));
        auto output_size = DSize(output().Width(), output().Height());
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"output_size", box_value(output_size));

        TryResizeView(resize_to_width, ActualHeight() + delta_input_height + delta_output_height);
    }

    input().Focus(FocusState::Programmatic);
}

template <typename XPanelHint>
auto winrt::tpcalc::implementation::MainPage::width_and_height(const XPanelHint& XPanel_hint, bool first_app_launch) -> DSize const {
// determine page width and height values that will accommodate an XPanel (vars or help panel).
// side effect: input and output elements may be reduced in height to make more space for the XPanel
    auto XPanel_height_space =
        ActualHeight()
        - calcPanel().ActualHeight()
        - bottomAppBar().ActualHeight();

    auto width = 0.0;
    if (first_app_launch)
        width = XPanel_hint.DWidth();
    else if (XPanel_height_space < 1 // should be XPanel().Visibility() == Visibility::Collapsed but XPanel().Visibility() is returning wrong value! why?!
                // < 1 for rounding/tolerance error (approx. value; empirically determined)
            && ActualWidth() < default_page_size.DWidth() + 3) // + 3 for rounding/tolerance error (approx. value; empirically determined)
        width = XPanel_hint.DWidth();
    else
        width = ActualWidth();

    auto height = 0.0;
    if (XPanel_height_space > XPanel_height_space_threshold && !first_app_launch)
        height = ActualHeight();
    else {
        auto input_output_extra_height =
            input().Height() + output().Height()
            - initial_input_size.DHeight() - initial_output_size.DHeight();
        height = default_page_size.Height + input_output_extra_height + XPanel_hint.Height;
    }

    auto max_width_height = max_page_size();
    if (width > max_width_height.DWidth())
        width = max_width_height.DWidth();
    if (height > max_width_height.DHeight())
        height = max_width_height.DHeight();

    auto new_XPanel_height_space =
        height
        - calcPanel().ActualHeight()
        - bottomAppBar().ActualHeight();

    if (new_XPanel_height_space <= XPanel_height_space_threshold) {
        auto factor = input().Height() > initial_input_size.DHeight() * min_in_out_down_scale_factor
            ? min_in_out_down_scale_factor : 1;
        input().Height(initial_input_size.DHeight() * factor);
        output().Height(initial_output_size.DHeight() * factor);
        outputType().Height(output().Height());
        input_height_limit_when_XPanel_showing = input().Height();
        output_height_limit_when_XPanel_showing = output().Height();
        auto input_size = DSize(input().Width(), input().Height());
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"input_size", box_value(input_size));
        auto output_size = DSize(output().Width(), output().Height());
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"output_size", box_value(output_size));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"input_height_limit_when_XPanel_showing", box_value(input_height_limit_when_XPanel_showing));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"output_height_limit_when_XPanel_showing", box_value(output_height_limit_when_XPanel_showing));
    }

    return {width, height};
}

DSize winrt::tpcalc::implementation::MainPage::max_page_size() {
// heuristic for maximum size that ApplicationView::TryResizePage will take;
// can't figure out how to determine actual size
    const auto& info = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
    auto max = DSize{
        info.ScreenWidthInRawPixels() / info.RawPixelsPerViewPixel() * 0.8,
        info.ScreenHeightInRawPixels() / info.RawPixelsPerViewPixel() * 0.8};
    if (max.DWidth() <= ActualWidth())
        max.DWidth(ActualWidth());
    if (max.DHeight() <= ActualHeight())
        max.DHeight(ActualHeight());
    return max;
}

void winrt::tpcalc::implementation::MainPage::output_PointerEntered(winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& /*e*/)
{
    auto visibility = Visibility::Visible;
    if (outputted_last_val) {
        // see set_output_to_last_val where outputTip text is set
        if (outputter.radix() == radices::decimal || outputter.radix() == radices::base10)
            visibility = Visibility::Collapsed;
    } else {
        // see set_output_to where outputTip text is set
        if (!output().IsTextTrimmed())
            visibility = Visibility::Collapsed;
    }
    outputTip.Visibility(visibility);
}
