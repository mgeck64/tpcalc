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

    resizing_to = DSize(ActualWidth(), ActualHeight());
    default_page_size = DSize(calcPanel().ActualWidth(), calcPanel().ActualHeight() + bottomAppBar().ActualHeight());
    initial_calcPanel_size = DSize(calcPanel().ActualWidth(), calcPanel().ActualHeight());
    initial_input_size = DSize(input().Width(), input().Height());
    initial_output_size = DSize(output().Width(), output().Height()); // output.ActualWidth() is returning 0 for some unknown reason
    page_resized_for_no_XPanel = unbox_value_or(local_settings.Values().Lookup(L"page_resized_for_no_XPanel"), default_page_size);
    base_input_size = unbox_value_or(local_settings.Values().Lookup(L"base_input_size"), initial_input_size);
    base_output_size = unbox_value_or(local_settings.Values().Lookup(L"base_output_size"), initial_output_size);
    minimize_input_output = unbox_value_or(local_settings.Values().Lookup(L"minimize_input_output"), minimize_input_output);
    size_values_valid = true;

    ApplicationView::GetForCurrentView().SetPreferredMinSize(default_page_size);
    if (!local_settings.Values().HasKey(L"Launched")) {
        local_settings.Values().Insert(L"Launched", box_value(true));
        show_help(L"help_quick_start_guide");
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
{
    if (!page_SizeChanged_update_layout)
        return;

    update_page_layout();

    if (!size_values_valid)
        ;
    else if ((fabs(resizing_to.width() - ActualWidth()) < 2)
            && (fabs(resizing_to.height() - ActualHeight()) < 2))
        ; // assume this is a programmatic change, which we don't want to track
    else if (varsPanel().Visibility() == Visibility::Visible) {
        assert(helpPanel().Visibility() == Visibility::Collapsed);
    } else if (helpPanel().Visibility() == Visibility::Visible) {
        assert(varsPanel().Visibility() == Visibility::Collapsed);
    } else {
        page_resized_for_no_XPanel = DSize(ActualWidth(), ActualHeight());
        base_input_size = DSize(input().Width(), input().Height());
        base_output_size = DSize(output().Width(), output().Height());
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"page_resized_for_no_XPanel", box_value(page_resized_for_no_XPanel));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"base_input_size", box_value(base_input_size));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"base_output_size", box_value(base_output_size));
    }
}

void winrt::tpcalc::implementation::MainPage::set_output_to_last_val() {
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

void winrt::tpcalc::implementation::MainPage::set_output_to_text(std::wstring_view text) {
    // if outputType is showing, hide it
    if (outputType().Visibility() == Visibility::Visible) {
        outputType().Visibility(Visibility::Collapsed);
        output().Width(output().Width() + outputType().Width());
    }

    output().Text(text);
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

    { // min_max_in_out_Button()
        auto tag = unbox_value_or(min_max_in_out_Button().Tag(), L"");
        if (input().Height() > initial_input_size.height()) {
            if (tag != L"min_in_out") {
                min_max_in_out_Button().Tag(box_value(L"min_in_out"));
                min_max_in_out_Button().Content(box_value(L"▲"));
                min_max_in_out_Button().IsEnabled(true);
            }
        } else {
            if (tag != L"not_min_in_out") {
                min_max_in_out_Button().Tag(box_value(L"not_min_in_out"));
                min_max_in_out_Button().Content(box_value(L"▼"));
                min_max_in_out_Button().IsEnabled(true);
            }
        }
    }
}

void winrt::tpcalc::implementation::MainPage::update_page_layout() {
    if (size_values_valid) {
        // dynamically size page sections:

        bool XPanel_visible = XPanel().Visibility() == Visibility::Visible;

       if (!XPanel_visible)
            minimize_input_output = fabs(default_page_size.Width - page_resized_for_no_XPanel.Width) < 2
               && fabs(default_page_size.Height - page_resized_for_no_XPanel.Height) < 2;

        double extra_height = 
            ActualHeight() -
            initial_calcPanel_size.height() -
            bottomAppBar().ActualHeight();
        if (extra_height < 0)
            extra_height = 0;

        assert(!(helpPanel().Visibility() == Visibility::Visible
            && varsPanel().Visibility() == Visibility::Visible));
        DSize XPanel_hint;
        if (helpPanel().Visibility() == Visibility::Visible)
            XPanel_hint = XPanel_hint_for_help;
        else if (varsPanel().Visibility() == Visibility::Visible)
            XPanel_hint = XPanel_hint_for_vars;

        double extra_height_for_input_output = std::max(extra_height - XPanel_hint.height() / 2, 0.0);
        
        double extra_height_for_input = extra_height_for_input_output / 2;
        if (XPanel_visible)
            if (base_input_size.height() < (initial_input_size.height() + extra_height_for_input))
                extra_height_for_input = std::max(base_input_size.height() - initial_input_size.height(), 0.0);
        double extra_height_for_output = std::max(extra_height_for_input_output - extra_height_for_input, 0.0);
        if (XPanel_visible)
            if (base_input_size.height() < (initial_output_size.height() + extra_height_for_output))
                extra_height_for_output = std::max(base_output_size.height() - initial_output_size.height(), 0.0);

        if (minimize_input_output) {
            extra_height_for_input = 0;
            extra_height_for_output = 0;
        }

        input().Height(initial_input_size.height() + extra_height_for_input);
        if (calc_util::almost_equal(extra_height_for_input, 0.0))
            input().TextWrapping(TextWrapping::NoWrap);
        else
            input().TextWrapping(TextWrapping::Wrap);

        output().Height(initial_output_size.height() + extra_height_for_output);
        outputType().Height(output().Height());

        auto space_for_XPanel = extra_height - extra_height_for_input - extra_height_for_output;
        if (space_for_XPanel > 16) // extra space for bottom margin; can't make this work in XAML
            space_for_XPanel -= 16;
        else
            space_for_XPanel = 0;
        if (varsPanel().Visibility() == Visibility::Visible)
            varsPanel().Height(space_for_XPanel);
        else
            varsPanel().Height(0);
        if (helpPanel().Visibility() == Visibility::Visible)
            helpPanel().Height(space_for_XPanel);
        else
            helpPanel().Height(0);

        auto delta_width = ActualWidth() - default_page_size.width();
        if (initial_calcPanel_size.width() + delta_width >= 0)
            calcPanel().Width(initial_calcPanel_size.width() + delta_width);
        if (initial_input_size.width() + delta_width >= 0)
            input().Width(initial_input_size.width() + delta_width);

        auto output_padding = outputType().Visibility() == Visibility::Visible ? 0 : outputType().Width();
        if (initial_output_size.width() + delta_width + output_padding >= 0)
            output().Width(initial_output_size.width() + delta_width + output_padding);

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

void winrt::tpcalc::implementation::MainPage::eval_input()
{
    auto input_str = input().Text();
    bool input_is_blank = false;
    try {
        input_is_blank = !parser.eval(input_str.c_str());
        set_output_to_last_val();
        input().Text(L""); // clear for next input (even if input_is_blank is true, input may have whitespace chars)
    } catch (const parser_type::parse_error& e) {
        set_output_to_text(e.error_str());
        if (e.view_is_valid_for(input_str.c_str()))
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
        last_inputs_idx = last_inputs.size();
    }
}

void winrt::tpcalc::implementation::MainPage::page_KeyDown(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& /*e*/)
{
}

void winrt::tpcalc::implementation::MainPage::input_KeyDown(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e)
{
    using namespace winrt::Windows::UI::Core;
    auto it = CoreWindow::GetForCurrentThread();
    enum {shift_flag = 1, ctrl_flag = 2, alt_flag = 4}; // bit flags
    auto shift_down = ((it.GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down) ? shift_flag : 0;
    auto ctrl_down = ((it.GetKeyState(VirtualKey::Control) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down) ? ctrl_flag : 0;
    auto alt_down = ((it.GetKeyState(VirtualKey::Menu) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down) ? alt_flag : 0;
    auto modifiers_down = shift_down | ctrl_down | alt_down;

    if (e.Key() == VirtualKey::Enter && !modifiers_down) {
        eval_input();
    } else if ((e.Key() == VirtualKey::F3) && !modifiers_down) { // recall last input
        input().SelectedText(last_input); // should insert new_text at caret position if nothing was selected
        input().Select(input().SelectionStart() + last_input.size(), 0);
    } else if ((e.Key() == VirtualKey::Up) && (modifiers_down == alt_flag)) {
        if (last_inputs_idx > 0) {
            --last_inputs_idx;
            auto& text = last_inputs[last_inputs_idx];
            input().Text(text);
            input().Select(text.size(), 0); // place caret at end
        }
    } else if ((e.Key() == VirtualKey::Down) && (modifiers_down == alt_flag)) {
        if (last_inputs_idx + 1 < last_inputs.size()) {
            ++last_inputs_idx;
            auto& text = last_inputs[last_inputs_idx];
            input().Text(text);
            input().Select(text.size(), 0); // place caret at end
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
        size.width(default_page_size.Width);
    if (size.Height < default_page_size.Height)
        size.height(default_page_size.Height);

    // max width/height heuristic:
    // windows TryResizeView(size) does nothing if size is too large
    auto excess = calc_util::exceeds_max_page_size(size);
    size.Width -= excess.Width;
    size.Height -= excess.Height;

    page_SizeChanged_update_layout = false;
    // we'll call update_page_layout here incase TryResizeView doesn't trigger
    // page_SizeChanged
    calc_util::make_resetter(page_SizeChanged_update_layout, true);
    resizing_to = size;
    ApplicationView::GetForCurrentView().TryResizeView(size); // note: can't rely on return value
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

void winrt::tpcalc::implementation::MainPage::show_vars(std::wstring_view tag) {
    if (tag == L"vars_show") {
        varsPanel().Visibility(Visibility::Visible);
        helpPanel().Visibility(Visibility::Collapsed);
        XPanel().Visibility(Visibility::Visible);
        TryResizeView(width_and_height(XPanel_hint_for_vars));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"XPanel", box_value(L"vars"));
    } else if (tag == L"vars_hide") {
        varsPanel().Visibility(Visibility::Collapsed);
        helpPanel().Visibility(Visibility::Collapsed);
        XPanel().Visibility(Visibility::Collapsed);
        TryResizeView(page_resized_for_no_XPanel);
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

void winrt::tpcalc::implementation::MainPage::show_help(std::wstring_view tag) {
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
    } else if (help_statistical_functions().Visibility() == Visibility::Visible) {
        visible = help_statistical_functions();
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
    } else if (tag == L"help_statistical_functions") {
        make_visible = help_statistical_functions();
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
        XPanel().Visibility(Visibility::Visible);
        XPanel_hint_for_help.height(calc_util::max_page_size().Height); // override with max page height
        TryResizeView(width_and_height(XPanel_hint_for_help));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"XPanel", box_value(tag));
    } else if (varsPanel().Visibility() == Visibility::Collapsed) {
        helpPanel().Visibility(Visibility::Collapsed);
        XPanel().Visibility(Visibility::Collapsed);
        TryResizeView(page_resized_for_no_XPanel);
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"XPanel", box_value(L"none"));
    }

    input().Focus(FocusState::Programmatic);
}

void winrt::tpcalc::implementation::MainPage::help_quick_start_guide_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_quick_start_guide");}

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

void winrt::tpcalc::implementation::MainPage::help_operator_precedence_and_associativity_Click(winrt::Windows::UI::Xaml::Documents::Hyperlink const&, winrt::Windows::UI::Xaml::Documents::HyperlinkClickEventArgs const&)
{show_help(L"help_operator_precedence_and_associativity");}

void winrt::tpcalc::implementation::MainPage::help_hide_help_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{show_help(L"help_hide_help");}

void winrt::tpcalc::implementation::MainPage::min_max_in_out_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&)
{
    auto tag = unbox_value_or(sender.as<FrameworkElement>().Tag(), L"");
    if (tag == L"min_in_out") {
        minimize_input_output = true;
        page_resized_for_no_XPanel.Height = default_page_size.Height;
        base_input_size = initial_input_size;
        base_output_size = initial_output_size;
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"minimize_input_output", box_value(minimize_input_output));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"page_resized_for_no_XPanel", box_value(page_resized_for_no_XPanel));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"base_input_size", box_value(base_input_size));
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"base_output_size", box_value(base_output_size));
        if (XPanel().Visibility() == Visibility::Collapsed)
            TryResizeView(page_resized_for_no_XPanel);
        else
            update_page_layout();
    } else if (tag == L"not_min_in_out") {
        minimize_input_output = false;
        Storage::ApplicationData::Current().LocalSettings().
            Values().Insert(L"minimize_input_output", box_value(minimize_input_output));

        if (calc_util::almost_equal(base_input_size.Height, initial_input_size.Height)) {
            assert(calc_util::almost_equal(base_output_size.Height, initial_output_size.Height));
            base_input_size.Height *= 3;
            base_output_size.Height += std::max(base_input_size.Height - initial_input_size.Height, 0.0f);
            auto extra_height_for_input_output = 
                (base_input_size.Height - initial_input_size.Height) +
                (base_output_size.Height - initial_output_size.Height);
            page_resized_for_no_XPanel.Height += extra_height_for_input_output;
            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"page_resized_for_no_XPanel", box_value(page_resized_for_no_XPanel));
            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"base_input_size", box_value(base_input_size));
            Storage::ApplicationData::Current().LocalSettings().
                Values().Insert(L"base_output_size", box_value(base_output_size));
        }

        DSize XPanel_hint;
        if (varsPanel().Visibility() == Visibility::Visible)
            XPanel_hint = XPanel_hint_for_vars;
        else if (helpPanel().Visibility() == Visibility::Visible)
            XPanel_hint = XPanel_hint_for_help;
        auto input_output_extra_height =
            base_input_size.height() + base_output_size.height()
            - initial_input_size.height() - initial_output_size.height();
        auto height_hint = default_page_size.height() + input_output_extra_height + XPanel_hint.height();
        if (ActualHeight() < height_hint)
            TryResizeView(ActualWidth(), height_hint);
        else
            update_page_layout();
    } else if (tag != L"none")
        assert(false); // missed one
    input().Focus(FocusState::Programmatic);
}

template <typename XPanelHint>
auto winrt::tpcalc::implementation::MainPage::width_and_height(const XPanelHint& XPanel_hint) -> DSize const {
    auto width = std::max(page_resized_for_no_XPanel.Width, XPanel_hint.Width);
    auto input_output_extra_height =
        base_input_size.Height + base_output_size.Height
        - initial_input_size.Height - initial_output_size.Height;
    auto height = default_page_size.Height + input_output_extra_height + XPanel_hint.Height;
    return {width, height};
}