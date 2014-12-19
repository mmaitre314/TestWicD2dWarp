//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "Test.h"

using namespace TestWicD2dWarp;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

void NTAPI EnableCallback(
    LPCGUID /*SourceId*/,
    ULONG IsEnabled,
    UCHAR Level,
    ULONGLONG /*MatchAnyKeyword*/,
    ULONGLONG /*MatchAllKeywords*/,
    PEVENT_FILTER_DESCRIPTOR /*FilterData*/,
    PVOID /*CallbackContext*/
    )
{
    switch (IsEnabled)
    {
    case EVENT_CONTROL_CODE_ENABLE_PROVIDER:
        g_bEnabled = TRUE;
        g_nLevel = Level;
        break;

    case EVENT_CONTROL_CODE_DISABLE_PROVIDER:
        g_bEnabled = FALSE;
        g_nLevel = 0;
        break;
    }
}

MainPage::MainPage()
{
    InitializeComponent();

    // {16F4A0EB-6913-4AEA-BE78-CE761108C479}
    static const GUID guidTrace = { 0x16f4a0eb, 0x6913, 0x4aea, { 0xbe, 0x78, 0xce, 0x76, 0x11, 0x8, 0xc4, 0x79 } };
    (void)EventRegister(&guidTrace, EnableCallback, NULL, &g_ETWHandle);

    Test::RunRGB();

    (void)EventUnregister(g_ETWHandle);
    g_ETWHandle = 0;
    g_bEnabled = false;
    g_nLevel = 0;
}
