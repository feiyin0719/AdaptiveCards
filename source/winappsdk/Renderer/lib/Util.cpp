// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "pch.h"

#include <regex>

#include "XamlHelpers.h"
#include "AdaptiveActionSetRenderer.h"
#include "AdaptiveColumnRenderer.h"
#include "AdaptiveColumnSetRenderer.h"

using namespace AdaptiveCards;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Data::Json;
using namespace ABI::Windows::UI;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace AdaptiveCards::Rendering::WinUI3;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;

HRESULT WStringToHString(std::wstring_view in, _Outptr_ HSTRING* out) noexcept
try
{
    if (out == nullptr)
    {
        return E_INVALIDARG;
    }
    else if (in.empty())
    {
        return WindowsCreateString(L"", 0, out);
    }
    else
    {
        return WindowsCreateString(in.data(), static_cast<UINT32>(in.length()), out);
    }
}
CATCH_RETURN();

std::string WStringToString(std::wstring_view in)
{
    const int length_in = static_cast<int>(in.length());

    if (length_in > 0)
    {
        const int length_out = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, in.data(), length_in, NULL, 0, NULL, NULL);

        if (length_out > 0)
        {
            std::string out(length_out, '\0');

            const int length_written =
                ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, in.data(), length_in, out.data(), length_out, NULL, NULL);

            if (length_written == length_out)
            {
                return out;
            }
        }

        throw bad_string_conversion();
    }

    return {};
}

std::wstring StringToWString(std::string_view in)
{
    const int length_in = static_cast<int>(in.length());

    if (length_in > 0)
    {
        const int length_out = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in.data(), length_in, NULL, 0);

        if (length_out > 0)
        {
            std::wstring out(length_out, L'\0');

            const int length_written =
                ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in.data(), length_in, out.data(), length_out);

            if (length_written == length_out)
            {
                return out;
            }
        }

        throw bad_string_conversion();
    }

    return {};
}

winrt::hstring UTF8ToHString(std::string_view in)
{
    return winrt::hstring{StringToWString(in)};
}

HRESULT UTF8ToHString(std::string_view in, _Outptr_ HSTRING* out) noexcept
try
{
    if (out == nullptr)
    {
        return E_INVALIDARG;
    }
    else
    {
        std::wstring wide = StringToWString(in);
        return WindowsCreateString(wide.c_str(), static_cast<UINT32>(wide.length()), out);
    }
}
CATCH_RETURN();

HRESULT HStringToUTF8(HSTRING in, std::string& out) noexcept
try
{
    UINT32 length = 0U;
    const auto* ptr_wide = WindowsGetStringRawBuffer(in, &length);
    out = WStringToString(std::wstring_view(ptr_wide, length));

    return S_OK;
}
CATCH_RETURN();

std::string HStringToUTF8(HSTRING in)
{
    std::string typeAsKey;
    if (SUCCEEDED(HStringToUTF8(in, typeAsKey)))
    {
        return typeAsKey;
    }

    return {};
}

std::string HStringToUTF8(winrt::hstring const& in)
{
    return WStringToString(in);
}

winrt::Windows::UI::Color GetColorFromString(std::string const& colorString)
{
    ABI::Windows::UI::Color output;
    THROW_IF_FAILED(GetColorFromString(colorString, &output));
    return reinterpret_cast<winrt::Windows::UI::Color&>(output);
}

// Get a Color object from color string
// Expected formats are "#AARRGGBB" (with alpha channel) and "#RRGGBB" (without alpha channel)
HRESULT GetColorFromString(const std::string& colorString, _Out_ ABI::Windows::UI::Color* color) noexcept
try
{
    if (colorString.length() > 0 && colorString.front() == '#')
    {
        // Get the pure hex value (without #)
        std::string hexColorString = colorString.substr(1, std::string::npos);

        std::regex colorWithAlphaRegex("[0-9a-f]{8}", std::regex_constants::icase);
        if (regex_match(hexColorString, colorWithAlphaRegex))
        {
            // If color string has alpha channel, extract and set to color
            std::string alphaString = hexColorString.substr(0, 2);
            INT32 alpha = strtol(alphaString.c_str(), nullptr, 16);

            color->A = static_cast<BYTE>(alpha);

            hexColorString = hexColorString.substr(2, std::string::npos);
        }
        else
        {
            // Otherwise, set full opacity
            std::string alphaString = "FF";
            INT32 alpha = strtol(alphaString.c_str(), nullptr, 16);
            color->A = static_cast<BYTE>(alpha);
        }

        // A valid string at this point should have 6 hex characters (RRGGBB)
        std::regex colorWithoutAlphaRegex("[0-9a-f]{6}", std::regex_constants::icase);
        if (regex_match(hexColorString, colorWithoutAlphaRegex))
        {
            // Then set all other Red, Green, and Blue channels
            std::string redString = hexColorString.substr(0, 2);
            INT32 red = strtol(redString.c_str(), nullptr, 16);

            std::string greenString = hexColorString.substr(2, 2);
            INT32 green = strtol(greenString.c_str(), nullptr, 16);

            std::string blueString = hexColorString.substr(4, 2);
            INT32 blue = strtol(blueString.c_str(), nullptr, 16);

            color->R = static_cast<BYTE>(red);
            color->G = static_cast<BYTE>(green);
            color->B = static_cast<BYTE>(blue);

            return S_OK;
        }
    }

    // All other formats are ignored (set alpha to 0)
    color->A = static_cast<BYTE>(0);

    return S_OK;
}
CATCH_RETURN();

HRESULT GetContainerStyleDefinition(ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle style,
                                    _In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                                    _Outptr_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveContainerStyleDefinition** styleDefinition) noexcept
try
{
    ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveContainerStylesDefinition> containerStyles;
    RETURN_IF_FAILED(hostConfig->get_ContainerStyles(&containerStyles));

    switch (style)
    {
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle::Accent:
        RETURN_IF_FAILED(containerStyles->get_Accent(styleDefinition));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle::Attention:
        RETURN_IF_FAILED(containerStyles->get_Attention(styleDefinition));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle::Emphasis:
        RETURN_IF_FAILED(containerStyles->get_Emphasis(styleDefinition));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle::Good:
        RETURN_IF_FAILED(containerStyles->get_Good(styleDefinition));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle::Warning:
        RETURN_IF_FAILED(containerStyles->get_Warning(styleDefinition));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle::Default:
    default:
        RETURN_IF_FAILED(containerStyles->get_Default(styleDefinition));
        break;
    }
    return S_OK;
}
CATCH_RETURN();

rtrender::AdaptiveContainerStyleDefinition GetContainerStyleDefinition(rtom::ContainerStyle const& style,
                                                                       rtrender::AdaptiveHostConfig const& hostConfig)
{
    /*ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveContainerStylesDefinition> containerStyles;
    RETURN_IF_FAILED(hostConfig->get_ContainerStyles(&containerStyles));*/
    auto containerStyles = hostConfig.ContainerStyles();

    switch (style)
    {
    case rtom::ContainerStyle::Accent:
        return containerStyles.Accent();
    case rtom::ContainerStyle::Attention:
        return containerStyles.Attention();
    case rtom::ContainerStyle::Emphasis:
        return containerStyles.Emphasis();
    case rtom::ContainerStyle::Good:
        return containerStyles.Good();
    case rtom::ContainerStyle::Warning:
        return containerStyles.Warning();
    case rtom::ContainerStyle::Default:
    default:
        return containerStyles.Default();
    }
}

HRESULT GetColorFromAdaptiveColor(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                                  ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor adaptiveColor,
                                  ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle containerStyle,
                                  bool isSubtle,
                                  bool highlight,
                                  _Out_ ABI::Windows::UI::Color* uiColor) noexcept
try
{
    ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveContainerStyleDefinition> styleDefinition;
    GetContainerStyleDefinition(containerStyle, hostConfig, &styleDefinition);

    ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveColorsConfig> colorsConfig;
    RETURN_IF_FAILED(styleDefinition->get_ForegroundColors(&colorsConfig));

    ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveColorConfig> colorConfig;
    switch (adaptiveColor)
    {
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor::Accent:
        RETURN_IF_FAILED(colorsConfig->get_Accent(&colorConfig));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor::Dark:
        RETURN_IF_FAILED(colorsConfig->get_Dark(&colorConfig));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor::Light:
        RETURN_IF_FAILED(colorsConfig->get_Light(&colorConfig));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor::Good:
        RETURN_IF_FAILED(colorsConfig->get_Good(&colorConfig));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor::Warning:
        RETURN_IF_FAILED(colorsConfig->get_Warning(&colorConfig));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor::Attention:
        RETURN_IF_FAILED(colorsConfig->get_Attention(&colorConfig));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor::Default:
    default:
        RETURN_IF_FAILED(colorsConfig->get_Default(&colorConfig));
        break;
    }

    if (highlight)
    {
        ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHighlightColorConfig> highlightColorConfig;
        RETURN_IF_FAILED(colorConfig->get_HighlightColors(&highlightColorConfig));
        RETURN_IF_FAILED(isSubtle ? highlightColorConfig->get_Subtle(uiColor) : highlightColorConfig->get_Default(uiColor));
    }
    else
    {
        RETURN_IF_FAILED(isSubtle ? colorConfig->get_Subtle(uiColor) : colorConfig->get_Default(uiColor));
    }

    return S_OK;
}
CATCH_RETURN();

HRESULT GetHighlighter(_In_ ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveTextElement* adaptiveTextElement,
                       _In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveRenderContext* renderContext,
                       _In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveRenderArgs* renderArgs,
                       _Out_ ABI::Windows::UI::Xaml::Documents::ITextHighlighter** textHighlighter) noexcept
{
    ComPtr<ABI::Windows::UI::Xaml::Documents::ITextHighlighter> localTextHighlighter =
        XamlHelpers::CreateABIClass<ABI::Windows::UI::Xaml::Documents::ITextHighlighter>(
            HStringReference(RuntimeClass_Windows_UI_Xaml_Documents_TextHighlighter));

    ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig> hostConfig;
    RETURN_IF_FAILED(renderContext->get_HostConfig(&hostConfig));

    ComPtr<IReference<ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor>> adaptiveForegroundColorRef;
    RETURN_IF_FAILED(adaptiveTextElement->get_Color(&adaptiveForegroundColorRef));

    ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor adaptiveForegroundColor =
        ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor::Default;
    if (adaptiveForegroundColorRef != nullptr)
    {
        adaptiveForegroundColorRef->get_Value(&adaptiveForegroundColor);
    }

    ComPtr<IReference<bool>> isSubtleRef;
    RETURN_IF_FAILED(adaptiveTextElement->get_IsSubtle(&isSubtleRef));

    boolean isSubtle = false;
    if (isSubtleRef != nullptr)
    {
        isSubtleRef->get_Value(&isSubtle);
    }

    ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle containerStyle;
    RETURN_IF_FAILED(renderArgs->get_ContainerStyle(&containerStyle));

    ABI::Windows::UI::Color backgroundColor;
    RETURN_IF_FAILED(GetColorFromAdaptiveColor(hostConfig.Get(), adaptiveForegroundColor, containerStyle, isSubtle, true, &backgroundColor));

    ABI::Windows::UI::Color foregroundColor;
    RETURN_IF_FAILED(GetColorFromAdaptiveColor(hostConfig.Get(), adaptiveForegroundColor, containerStyle, isSubtle, false, &foregroundColor));

    RETURN_IF_FAILED(localTextHighlighter->put_Background(XamlHelpers::GetSolidColorBrush(backgroundColor).Get()));
    RETURN_IF_FAILED(localTextHighlighter->put_Foreground(XamlHelpers::GetSolidColorBrush(foregroundColor).Get()));

    localTextHighlighter.CopyTo(textHighlighter);
    return S_OK;
}

namespace rtrender = winrt::AdaptiveCards::Rendering::WinUI3;
namespace rtom = winrt::AdaptiveCards::ObjectModel::WinUI3;

uint32_t GetSpacingSizeFromSpacing(rtrender::AdaptiveHostConfig const& hostConfig, rtom::Spacing const& spacing)
{
    auto spacingConfig = hostConfig.Spacing();

    switch (spacing)
    {
    case winrt::AdaptiveCards::ObjectModel::WinUI3::Spacing::None:
        return 0;
    case winrt::AdaptiveCards::ObjectModel::WinUI3::Spacing::Small:
        return spacingConfig.Small();
    case winrt::AdaptiveCards::ObjectModel::WinUI3::Spacing::Medium:
        return spacingConfig.Medium();
    case winrt::AdaptiveCards::ObjectModel::WinUI3::Spacing::Large:
        return spacingConfig.Large();
    case winrt::AdaptiveCards::ObjectModel::WinUI3::Spacing::ExtraLarge:
        return spacingConfig.ExtraLarge();
    case winrt::AdaptiveCards::ObjectModel::WinUI3::Spacing::Padding:
        return spacingConfig.Padding();
    case winrt::AdaptiveCards::ObjectModel::WinUI3::Spacing::Default:
    default:
        return spacingConfig.Default();
    }
}

HRESULT GetSpacingSizeFromSpacing(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                                  ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing spacing,
                                  _Out_ UINT* spacingSize) noexcept
try
{
    ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveSpacingConfig> spacingConfig;
    RETURN_IF_FAILED(hostConfig->get_Spacing(&spacingConfig));

    switch (spacing)
    {
    case ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing::None:
        *spacingSize = 0;
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing::Small:
        RETURN_IF_FAILED(spacingConfig->get_Small(spacingSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing::Medium:
        RETURN_IF_FAILED(spacingConfig->get_Medium(spacingSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing::Large:
        RETURN_IF_FAILED(spacingConfig->get_Large(spacingSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing::ExtraLarge:
        RETURN_IF_FAILED(spacingConfig->get_ExtraLarge(spacingSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing::Padding:
        RETURN_IF_FAILED(spacingConfig->get_Padding(spacingSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing::Default:
    default:
        RETURN_IF_FAILED(spacingConfig->get_Default(spacingSize));
        break;
    }

    return S_OK;
}
CATCH_RETURN();

HRESULT GetBackgroundColorFromStyle(ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle style,
                                    _In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                                    _Out_ ABI::Windows::UI::Color* backgroundColor) noexcept
try
{
    ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveContainerStyleDefinition> styleDefinition;
    RETURN_IF_FAILED(GetContainerStyleDefinition(style, hostConfig, &styleDefinition));
    RETURN_IF_FAILED(styleDefinition->get_BackgroundColor(backgroundColor));

    return S_OK;
}
CATCH_RETURN();

winrt::Windows::UI::Color GetBackgroundColorFromStyle(rtom::ContainerStyle const& style, rtrender::AdaptiveHostConfig const& hostConfig)
{
    auto styleDefinition = GetContainerStyleDefinition(style, hostConfig);
    return styleDefinition.BackgroundColor();
}

HRESULT GetBorderColorFromStyle(ABI::AdaptiveCards::ObjectModel::WinUI3::ContainerStyle style,
                                _In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                                _Out_ ABI::Windows::UI::Color* borderColor) noexcept
try
{
    ComPtr<ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveContainerStyleDefinition> styleDefinition;
    RETURN_IF_FAILED(GetContainerStyleDefinition(style, hostConfig, &styleDefinition));
    RETURN_IF_FAILED(styleDefinition->get_BorderColor(borderColor));

    return S_OK;
}
CATCH_RETURN();

HRESULT GetFontDataFromFontType(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                                ABI::AdaptiveCards::ObjectModel::WinUI3::FontType fontType,
                                ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize desiredSize,
                                ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight desiredWeight,
                                _Outptr_ HSTRING* resultFontFamilyName,
                                _Out_ UINT32* resultSize,
                                _Out_ ABI::Windows::UI::Text::FontWeight* resultWeight) noexcept
try
{
    RETURN_IF_FAILED(GetFontFamilyFromFontType(hostConfig, fontType, resultFontFamilyName));
    RETURN_IF_FAILED(GetFontSizeFromFontType(hostConfig, fontType, desiredSize, resultSize));
    RETURN_IF_FAILED(GetFontWeightFromStyle(hostConfig, fontType, desiredWeight, resultWeight));
    return S_OK;
}
CATCH_RETURN();

HRESULT GetFontFamilyFromFontType(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                                  ABI::AdaptiveCards::ObjectModel::WinUI3::FontType fontType,
                                  _Outptr_ HSTRING* resultFontFamilyName) noexcept
try
{
    HString result;
    ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontTypeDefinition* typeDefinition;

    // get FontFamily from desired style
    RETURN_IF_FAILED(GetFontType(hostConfig, fontType, &typeDefinition));
    RETURN_IF_FAILED(typeDefinition->get_FontFamily(result.GetAddressOf()));
    if (result == NULL)
    {
        if (fontType == ABI::AdaptiveCards::ObjectModel::WinUI3::FontType::Monospace)
        {
            // fallback to system default monospace FontFamily
            RETURN_IF_FAILED(UTF8ToHString("Courier New", result.GetAddressOf()));
        }
        else
        {
            // fallback to deprecated FontFamily
            RETURN_IF_FAILED(hostConfig->get_FontFamily(result.GetAddressOf()));
            if (result == NULL)
            {
                // fallback to system default FontFamily
                RETURN_IF_FAILED(UTF8ToHString("Segoe UI", result.GetAddressOf()));
            }
        }
    }
    return result.CopyTo(resultFontFamilyName);
}
CATCH_RETURN();

HRESULT GetFontSizeFromFontType(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                                ABI::AdaptiveCards::ObjectModel::WinUI3::FontType fontType,
                                ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize desiredSize,
                                _Out_ UINT32* resultSize) noexcept
try
{
    UINT32 result;
    ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontTypeDefinition* fontTypeDefinition;
    ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontSizesConfig* sizesConfig;

    // get FontSize from desired style
    RETURN_IF_FAILED(GetFontType(hostConfig, fontType, &fontTypeDefinition));
    RETURN_IF_FAILED(fontTypeDefinition->get_FontSizes(&sizesConfig));
    RETURN_IF_FAILED(GetFontSize(sizesConfig, desiredSize, &result));

    if (result == MAXUINT32)
    {
        // get FontSize from Default style
        RETURN_IF_FAILED(GetFontType(hostConfig, ABI::AdaptiveCards::ObjectModel::WinUI3::FontType::Default, &fontTypeDefinition));
        RETURN_IF_FAILED(fontTypeDefinition->get_FontSizes(&sizesConfig));
        RETURN_IF_FAILED(GetFontSize(sizesConfig, desiredSize, &result));

        if (result == MAXUINT32)
        {
            // get deprecated FontSize
            RETURN_IF_FAILED(hostConfig->get_FontSizes(&sizesConfig));
            RETURN_IF_FAILED(GetFontSize(sizesConfig, desiredSize, &result));

            if (result == MAXUINT32)
            {
                // set system default FontSize based on desired style
                switch (desiredSize)
                {
                case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::Small:
                    result = 10;
                    break;
                case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::Medium:
                    result = 14;
                    break;
                case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::Large:
                    result = 17;
                    break;
                case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::ExtraLarge:
                    result = 20;
                    break;
                case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::Default:
                default:
                    result = 12;
                    break;
                }
            }
        }
    }
    *resultSize = result;
    return S_OK;
}
CATCH_RETURN();

HRESULT GetFontWeightFromStyle(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                               ABI::AdaptiveCards::ObjectModel::WinUI3::FontType fontType,
                               ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight desiredWeight,
                               _Out_ ABI::Windows::UI::Text::FontWeight* resultWeight) noexcept
try
{
    UINT16 result;
    ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontTypeDefinition* typeDefinition;
    ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontWeightsConfig* weightConfig;

    // get FontWeight from desired fontType
    RETURN_IF_FAILED(GetFontType(hostConfig, fontType, &typeDefinition));
    RETURN_IF_FAILED(typeDefinition->get_FontWeights(&weightConfig));
    RETURN_IF_FAILED(GetFontWeight(weightConfig, desiredWeight, &result));

    if (result == MAXUINT16)
    {
        // get FontWeight from Default style
        RETURN_IF_FAILED(GetFontType(hostConfig, ABI::AdaptiveCards::ObjectModel::WinUI3::FontType::Default, &typeDefinition));
        RETURN_IF_FAILED(typeDefinition->get_FontWeights(&weightConfig));
        RETURN_IF_FAILED(GetFontWeight(weightConfig, desiredWeight, &result));

        if (result == MAXUINT16)
        {
            // get deprecated FontWeight
            RETURN_IF_FAILED(hostConfig->get_FontWeights(&weightConfig));
            RETURN_IF_FAILED(GetFontWeight(weightConfig, desiredWeight, &result));

            if (result == MAXUINT16)
            {
                // set system default FontWeight based on desired style
                switch (desiredWeight)
                {
                case ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight::Lighter:
                    result = 200;
                    break;
                case ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight::Bolder:
                    result = 800;
                    break;
                case ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight::Default:
                default:
                    result = 400;
                    break;
                }
            }
        }
    }
    resultWeight->Weight = result;
    return S_OK;
}
CATCH_RETURN();

HRESULT GetFontType(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                    ABI::AdaptiveCards::ObjectModel::WinUI3::FontType fontType,
                    _COM_Outptr_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontTypeDefinition** fontTypeDefinition) noexcept
try
{
    ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontTypesDefinition* fontTypes;
    RETURN_IF_FAILED(hostConfig->get_FontTypes(&fontTypes));

    switch (fontType)
    {
    case ABI::AdaptiveCards::ObjectModel::WinUI3::FontType::Monospace:
        RETURN_IF_FAILED(fontTypes->get_Monospace(fontTypeDefinition));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::FontType::Default:
    default:
        RETURN_IF_FAILED(fontTypes->get_Default(fontTypeDefinition));
        break;
    }
    return S_OK;
}
CATCH_RETURN();

HRESULT GetFontSize(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontSizesConfig* sizesConfig,
                    ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize desiredSize,
                    _Out_ UINT32* resultSize) noexcept
try
{
    switch (desiredSize)
    {
    case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::Small:
        RETURN_IF_FAILED(sizesConfig->get_Small(resultSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::Medium:
        RETURN_IF_FAILED(sizesConfig->get_Medium(resultSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::Large:
        RETURN_IF_FAILED(sizesConfig->get_Large(resultSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::ExtraLarge:
        RETURN_IF_FAILED(sizesConfig->get_ExtraLarge(resultSize));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize::Default:
    default:
        RETURN_IF_FAILED(sizesConfig->get_Default(resultSize));
        break;
    }
    return S_OK;
}
CATCH_RETURN();

HRESULT GetFontWeight(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFontWeightsConfig* weightsConfig,
                      ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight desiredWeight,
                      _Out_ UINT16* resultWeight) noexcept
try
{
    switch (desiredWeight)
    {
    case ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight::Lighter:
        RETURN_IF_FAILED(weightsConfig->get_Lighter(resultWeight));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight::Bolder:
        RETURN_IF_FAILED(weightsConfig->get_Bolder(resultWeight));
        break;
    case ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight::Default:
    default:
        RETURN_IF_FAILED(weightsConfig->get_Default(resultWeight));
        break;
    }
    return S_OK;
}
CATCH_RETURN();

winrt::Windows::Data::Json::JsonObject StringToJsonObject(const std::string& inputString)
{
    return HStringToJsonObject(UTF8ToHString(inputString));
}

winrt::Windows::Data::Json::JsonObject HStringToJsonObject(winrt::hstring const& inputHString)
{
    winrt::Windows::Data::Json::JsonObject obj{nullptr};

    if (!winrt::Windows::Data::Json::JsonObject::TryParse(inputHString, obj))
    {
        obj = {};
    }

    return obj;
}

std::string JsonObjectToString(winrt::Windows::Data::Json::JsonObject const& inputJson)
{
    return HStringToUTF8(JsonObjectToHString(inputJson));
}

winrt::hstring JsonObjectToHString(winrt::Windows::Data::Json::JsonObject const& inputJson)
{
    return inputJson.Stringify();
}

HRESULT StringToJsonObject(const std::string& inputString, _COM_Outptr_ IJsonObject** result)
{
    HString asHstring;
    RETURN_IF_FAILED(UTF8ToHString(inputString, asHstring.GetAddressOf()));
    return HStringToJsonObject(asHstring.Get(), result);
}

HRESULT HStringToJsonObject(const HSTRING& inputHString, _COM_Outptr_ IJsonObject** result)
{
    ComPtr<IJsonObjectStatics> jObjectStatics;
    RETURN_IF_FAILED(GetActivationFactory(HStringReference(RuntimeClass_Windows_Data_Json_JsonObject).Get(), &jObjectStatics));
    ComPtr<IJsonObject> jObject;
    HRESULT hr = jObjectStatics->Parse(inputHString, &jObject);
    if (FAILED(hr))
    {
        RETURN_IF_FAILED(ActivateInstance(HStringReference(RuntimeClass_Windows_Data_Json_JsonObject).Get(), &jObject));
    }
    *result = jObject.Detach();
    return S_OK;
}

HRESULT JsonObjectToString(_In_ IJsonObject* inputJson, std::string& result)
{
    HString asHstring;
    RETURN_IF_FAILED(JsonObjectToHString(inputJson, asHstring.GetAddressOf()));
    return HStringToUTF8(asHstring.Get(), result);
}

HRESULT JsonObjectToHString(_In_ IJsonObject* inputJson, _Outptr_ HSTRING* result)
{
    if (!inputJson)
    {
        return E_INVALIDARG;
    }
    ComPtr<IJsonObject> localInputJson(inputJson);
    ComPtr<IJsonValue> asJsonValue;
    RETURN_IF_FAILED(localInputJson.As(&asJsonValue));
    return (asJsonValue->Stringify(result));
}

HRESULT StringToJsonValue(const std::string inputString, _COM_Outptr_ IJsonValue** result)
{
    HString asHstring;
    RETURN_IF_FAILED(UTF8ToHString(inputString, asHstring.GetAddressOf()));
    return HStringToJsonValue(asHstring.Get(), result);
}

HRESULT HStringToJsonValue(const HSTRING& inputHString, _COM_Outptr_ IJsonValue** result)
{
    ComPtr<IJsonValueStatics> jValueStatics;
    RETURN_IF_FAILED(GetActivationFactory(HStringReference(RuntimeClass_Windows_Data_Json_JsonValue).Get(), &jValueStatics));
    ComPtr<IJsonValue> jValue;
    HRESULT hr = jValueStatics->Parse(inputHString, &jValue);
    if (FAILED(hr))
    {
        RETURN_IF_FAILED(ActivateInstance(HStringReference(RuntimeClass_Windows_Data_Json_JsonValue).Get(), &jValue));
    }
    *result = jValue.Detach();
    return S_OK;
}

HRESULT JsonValueToString(_In_ IJsonValue* inputValue, std::string& result)
{
    HString asHstring;
    RETURN_IF_FAILED(JsonValueToHString(inputValue, asHstring.GetAddressOf()));
    return HStringToUTF8(asHstring.Get(), result);
}

HRESULT JsonValueToHString(_In_ IJsonValue* inputJsonValue, _Outptr_ HSTRING* result)
{
    if (!inputJsonValue)
    {
        return E_INVALIDARG;
    }
    ComPtr<IJsonValue> localInputJsonValue(inputJsonValue);
    return (localInputJsonValue->Stringify(result));
}

HRESULT JsonCppToJsonObject(const Json::Value& jsonCppValue, _COM_Outptr_ IJsonObject** result)
{
    std::string jsonString = ParseUtil::JsonToString(jsonCppValue);
    return StringToJsonObject(jsonString, result);
}

HRESULT JsonObjectToJsonCpp(_In_ ABI::Windows::Data::Json::IJsonObject* jsonObject, _Out_ Json::Value* jsonCppValue)
{
    std::string jsonString;
    RETURN_IF_FAILED(JsonObjectToString(jsonObject, jsonString));

    *jsonCppValue = ParseUtil::GetJsonValueFromString(jsonString);

    return S_OK;
}

HRESULT ProjectedActionTypeToHString(ABI::AdaptiveCards::ObjectModel::WinUI3::ElementType projectedActionType, _Outptr_ HSTRING* result)
{
    ActionType sharedActionType = static_cast<ActionType>(projectedActionType);
    return UTF8ToHString(ActionTypeToString(sharedActionType), result);
}

HRESULT ProjectedElementTypeToHString(ABI::AdaptiveCards::ObjectModel::WinUI3::ElementType projectedElementType, _Outptr_ HSTRING* result)
{
    CardElementType sharedElementType = static_cast<CardElementType>(projectedElementType);
    return UTF8ToHString(CardElementTypeToString(sharedElementType), result);
}

bool MeetsRequirements(rtom::IAdaptiveCardElement const& cardElement, rtrender::IAdaptiveFeatureRegistration const& featureRegistration)
{
    winrt::Windows::Foundation::Collections::IVector<rtom::AdaptiveRequirement> requirements = cardElement.Requirements();
    bool meetsRequirementsLocal = true;

    for (auto req : requirements)
    {
        // winrt::hstring name = req.Name();
        winrt::hstring registrationVersion = featureRegistration.Get(req.Name());

        if (registrationVersion.empty())
        {
            meetsRequirementsLocal = false;
        }
        else
        {
            std::string requirementVersionString = HStringToUTF8(req.Version());
            if (requirementVersionString != "*")
            {
                SemanticVersion requirementSemanticVersion(requirementVersionString);
                SemanticVersion registrationSemanticVersion(HStringToUTF8(registrationVersion));
                if (registrationSemanticVersion < requirementSemanticVersion)
                {
                    meetsRequirementsLocal = false;
                }
            }
        }
    }

    return meetsRequirementsLocal;
}

HRESULT MeetsRequirements(_In_ ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveCardElement* cardElement,
                          _In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveFeatureRegistration* featureRegistration,
                          _Out_ bool* meetsRequirements)
{
    ComPtr<IVector<ABI::AdaptiveCards::ObjectModel::WinUI3::AdaptiveRequirement*>> requirements;
    RETURN_IF_FAILED(cardElement->get_Requirements(&requirements));

    bool meetsRequirementsLocal = true;

    if (requirements != nullptr)
    {
        HRESULT hr =
            IterateOverVectorWithFailure<ABI::AdaptiveCards::ObjectModel::WinUI3::AdaptiveRequirement, ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveRequirement>(
                requirements.Get(),
                true,
                [&](ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveRequirement* requirement)
                {
                    HString name;
                    RETURN_IF_FAILED(requirement->get_Name(name.GetAddressOf()));

                    HString registrationVersion;
                    RETURN_IF_FAILED(featureRegistration->Get(name.Get(), registrationVersion.GetAddressOf()));

                    if (!registrationVersion.IsValid())
                    {
                        meetsRequirementsLocal = false;
                    }
                    else
                    {
                        HString requirementVersion;
                        RETURN_IF_FAILED(requirement->get_Version(requirementVersion.GetAddressOf()));

                        std::string requirementVersionString;
                        RETURN_IF_FAILED(HStringToUTF8(requirementVersion.Get(), requirementVersionString));

                        // "*" matches any version
                        if (requirementVersionString != "*")
                        {
                            SemanticVersion requirementSemanticVersion(HStringToUTF8(requirementVersion.Get()));
                            SemanticVersion registrationSemanticVersion(HStringToUTF8(registrationVersion.Get()));

                            if (registrationSemanticVersion < requirementSemanticVersion)
                            {
                                // host's provided version is too low
                                meetsRequirementsLocal = false;
                            }
                        }
                    }
                    return S_OK;
                });
        RETURN_IF_FAILED(hr);
    }
    *meetsRequirements = meetsRequirementsLocal;

    return S_OK;
}

HRESULT IsBackgroundImageValid(_In_ ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveBackgroundImage* backgroundImageElement,
                               _Out_ BOOL* isValid)
{
    *isValid = FALSE;
    ComPtr<ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveBackgroundImage> backgroundImage(backgroundImageElement);
    if (backgroundImage != NULL)
    {
        HString url;
        RETURN_IF_FAILED(backgroundImage->get_Url(url.GetAddressOf()));
        *isValid = url.IsValid();
    }
    return S_OK;
}

bool IsBackgroundImageValid(rtom::AdaptiveBackgroundImage backgroundImageElement)
{
    // what is the reason to create impl?
    auto backgroundImage = winrt::make_self<rtom::AdaptiveBackgroundImage>(backgroundImageElement);
    if (backgroundImage != NULL)
    {
        return !backgroundImage->Url().empty();
    }
    return false;
}

void GetUrlFromString(_In_ ABI::AdaptiveCards::Rendering::WinUI3::IAdaptiveHostConfig* hostConfig,
                      _In_ HSTRING urlString,
                      _Outptr_ ABI::Windows::Foundation::IUriRuntimeClass** url)
{
    ComPtr<ABI::Windows::Foundation::IUriRuntimeClassFactory> uriActivationFactory;
    THROW_IF_FAILED(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(), &uriActivationFactory));

    ComPtr<ABI::Windows::Foundation::IUriRuntimeClass> localUrl;

    // Try to treat URI as absolute
    boolean isUrlRelative = FAILED(uriActivationFactory->CreateUri(urlString, localUrl.GetAddressOf()));

    // Otherwise, try to treat URI as relative
    if (isUrlRelative)
    {
        HString imageBaseUrl;
        THROW_IF_FAILED(hostConfig->get_ImageBaseUrl(imageBaseUrl.GetAddressOf()));

        if (imageBaseUrl.Get() != nullptr)
        {
            THROW_IF_FAILED(uriActivationFactory->CreateWithRelativeUri(imageBaseUrl.Get(), urlString, localUrl.GetAddressOf()));
        }
    }

    THROW_IF_FAILED(localUrl.CopyTo(url));
}

Color GenerateLHoverColor(const Color& originalColor)
{
    const double hoverIncrement = 0.25;

    Color hoverColor;
    hoverColor.A = originalColor.A;
    hoverColor.R = originalColor.R - static_cast<BYTE>(originalColor.R * hoverIncrement);
    hoverColor.G = originalColor.G - static_cast<BYTE>(originalColor.G * hoverIncrement);
    hoverColor.B = originalColor.B - static_cast<BYTE>(originalColor.B * hoverIncrement);
    return hoverColor;
}

DateTime GetDateTime(unsigned int year, unsigned int month, unsigned int day)
{
    SYSTEMTIME systemTime = {(WORD)year, (WORD)month, 0, (WORD)day};

    // Convert to UTC
    TIME_ZONE_INFORMATION timeZone;
    GetTimeZoneInformation(&timeZone);
    TzSpecificLocalTimeToSystemTime(&timeZone, &systemTime, &systemTime);

    // Convert to ticks
    FILETIME fileTime;
    SystemTimeToFileTime(&systemTime, &fileTime);
    DateTime dateTime{(INT64)fileTime.dwLowDateTime + (((INT64)fileTime.dwHighDateTime) << 32)};

    return dateTime;
}

HRESULT GetDateTimeReference(unsigned int year, unsigned int month, unsigned int day, _COM_Outptr_ IReference<DateTime>** dateTimeReference)
{
    DateTime dateTime = GetDateTime(year, month, day);

    ComPtr<IPropertyValueStatics> factory;
    RETURN_IF_FAILED(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), &factory));

    ComPtr<IInspectable> inspectable;
    RETURN_IF_FAILED(factory->CreateDateTime(dateTime, &inspectable));

    ComPtr<IReference<DateTime>> localDateTimeReference;
    RETURN_IF_FAILED(inspectable.As(&localDateTimeReference));

    *dateTimeReference = localDateTimeReference.Detach();

    return S_OK;
}

HRESULT CopyTextElement(_In_ ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveTextElement* textElement,
                        _COM_Outptr_ ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveTextElement** copiedTextElement)
{
    ComPtr<ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveTextRun> localCopiedTextRun =
        XamlHelpers::CreateABIClass<ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveTextRun>(
            HStringReference(RuntimeClass_AdaptiveCards_ObjectModel_WinUI3_AdaptiveTextRun));

    ComPtr<ABI::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveTextElement> localCopiedTextElement;
    RETURN_IF_FAILED(localCopiedTextRun.As(&localCopiedTextElement));

    ComPtr<IReference<ABI::AdaptiveCards::ObjectModel::WinUI3::ForegroundColor>> color;
    RETURN_IF_FAILED(textElement->get_Color(&color));
    RETURN_IF_FAILED(localCopiedTextElement->put_Color(color.Get()));

    ComPtr<IReference<ABI::AdaptiveCards::ObjectModel::WinUI3::FontType>> fontType;
    RETURN_IF_FAILED(textElement->get_FontType(&fontType));
    RETURN_IF_FAILED(localCopiedTextElement->put_FontType(fontType.Get()));

    ComPtr<IReference<bool>> isSubtle;
    RETURN_IF_FAILED(textElement->get_IsSubtle(&isSubtle));
    RETURN_IF_FAILED(localCopiedTextElement->put_IsSubtle(isSubtle.Get()));

    HString language;
    RETURN_IF_FAILED(textElement->get_Language(language.GetAddressOf()));
    RETURN_IF_FAILED(localCopiedTextElement->put_Language(language.Get()));

    ComPtr<IReference<ABI::AdaptiveCards::ObjectModel::WinUI3::TextSize>> size;
    RETURN_IF_FAILED(textElement->get_Size(&size));
    RETURN_IF_FAILED(localCopiedTextElement->put_Size(size.Get()));

    ComPtr<IReference<ABI::AdaptiveCards::ObjectModel::WinUI3::TextWeight>> weight;
    RETURN_IF_FAILED(textElement->get_Weight(&weight));
    RETURN_IF_FAILED(localCopiedTextElement->put_Weight(weight.Get()));

    HString text;
    RETURN_IF_FAILED(textElement->get_Text(text.GetAddressOf()));
    RETURN_IF_FAILED(localCopiedTextElement->put_Text(text.Get()));

    RETURN_IF_FAILED(localCopiedTextElement.CopyTo(copiedTextElement));
    return S_OK;
}

void RegisterDefaultElementRenderers(rtrender::implementation::AdaptiveElementRendererRegistration* registration, XamlBuilder* xamlBuilder)
{
    registration->Set(L"ActionSet", winrt::make<rtrender::implementation::AdaptiveActionSetRenderer>());
    registration->Set(L"Column", winrt::make<rtrender::implementation::AdaptiveColumnRenderer>());
    registration->Set(L"ColumnSet", winrt::make<rtrender::implementation::AdaptiveColumnSetRenderer>());
    registration->Set(L"Container", winrt::make<rtrender::implementation::AdaptiveContainerRenderer>());
    registration->Set(L"FactSet", winrt::make<rtrender::implementation::AdaptiveFactSetRenderer>());
    registration->Set(L"Image", winrt::make<rtrender::implementation::AdaptiveImageRenderer>(xamlBuilder));
    registration->Set(L"ImageSet", winrt::make<rtrender::implementation::AdaptiveImageSetRenderer>());
    registration->Set(L"Input.ChoiceSet", winrt::make<rtrender::implementation::AdaptiveChoiceSetInputRenderer>());
    registration->Set(L"Input.Date", winrt::make<rtrender::implementation::AdaptiveDateInputRenderer>());
    registration->Set(L"Input.Number", winrt::make<rtrender::implementation::AdaptiveNumberInputRenderer>());
    registration->Set(L"Input.Text", winrt::make<rtrender::implementation::AdaptiveTextInputRenderer>());
    registration->Set(L"Input.Time", winrt::make<rtrender::implementation::AdaptiveTimeInputRenderer>());
    registration->Set(L"Input.Toggle", winrt::make<rtrender::implementation::AdaptiveToggleInputRenderer>());
    registration->Set(L"Media", winrt::make<rtrender::implementation::AdaptiveMediaRenderer>());
    registration->Set(L"RichTextBlock", winrt::make<rtrender::implementation::AdaptiveRichTextBlockRenderer>());
    registration->Set(L"Table", winrt::make<rtrender::implementation::AdaptiveTableRenderer>());
    registration->Set(L"TextBlock", winrt::make<rtrender::implementation::AdaptiveTextBlockRenderer>());
}

void RegisterDefaultActionRenderers(rtrender::implementation::AdaptiveActionRendererRegistration* registration)
{
    RETURN_IF_FAILED(registration->Set(HStringReference(L"Action.OpenUrl").Get(),
                                       Make<AdaptiveCards::Rendering::WinUI3::AdaptiveOpenUrlActionRenderer>().Get()));
    RETURN_IF_FAILED(registration->Set(HStringReference(L"Action.ShowCard").Get(),
                                       Make<AdaptiveCards::Rendering::WinUI3::AdaptiveShowCardActionRenderer>().Get()));
    RETURN_IF_FAILED(registration->Set(HStringReference(L"Action.Submit").Get(),
                                       Make<AdaptiveCards::Rendering::WinUI3::AdaptiveSubmitActionRenderer>().Get()));
    RETURN_IF_FAILED(registration->Set(HStringReference(L"Action.ToggleVisibility").Get(),
                                       Make<AdaptiveCards::Rendering::WinUI3::AdaptiveToggleVisibilityActionRenderer>().Get()));
    RETURN_IF_FAILED(registration->Set(HStringReference(L"Action.Execute").Get(),
                                       Make<AdaptiveCards::Rendering::WinUI3::AdaptiveExecuteActionRenderer>().Get()));
    return S_OK;
}
