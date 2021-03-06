#include "pdcd2d.h"

#include "deffont.h"

#include "ColorGlyphEffect.hpp"
#include <windowsx.h>
#include <memory>

#include <d3d11.h>
#include <wrl/client.h>
using namespace Microsoft::WRL; // for ComPtr

HWND PDC_d2d_hwnd = NULL;
ID2D1DeviceContext *PDC_d2d_context = NULL;
ID2D1Effect *PDC_d2d_colorEffect = NULL;
ID2D1Bitmap *PDC_d2d_font_bitmap = NULL;
IDXGISwapChain1 *PDC_d2d_swapChain = NULL;
D2D1::ColorF PDC_d2d_colors[];

static float PDC_d2d_dpi_x = 96.0f;
static float PDC_d2d_dpi_y = 96.0f;

int PDC_d2d_should_resize = 0;
int PDC_d2d_cols = 80;
int PDC_d2d_rows = 25;


const static LPCWSTR PDC_d2d_classname = L"MainWindow";

static void PDC_d2d_init_colours();
static bool PDC_d2d_load_font_from_memory();
static bool PDC_d2d_load_font_from_file();
static bool PDC_d2d_create_context();
static bool PDC_d2d_resize_swapchain(void);

// Get the HINSTANCE of the executable or dll we have been linked in to.
// https://blogs.msdn.microsoft.com/oldnewthing/20041025-00/?p=37483
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

bool PDC_can_change_color(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return TRUE;
}

int PDC_color_content(short color, short *red, short *green, short *blue)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return OK;
}

int PDC_init_color(short color, short red, short green, short blue)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return OK;
}

void PDC_reset_prog_mode(void)
{
    PDC_LOG(("PDC_reset_prog_mode() - called.\n"));
}

void PDC_reset_shell_mode(void)
{
    PDC_LOG(("PDC_reset_shell_mode() - called.\n"));
}

int PDC_resize_screen(int nlines, int ncols)
{

    if(nlines == 0 && ncols == 0){
        PDC_d2d_resize_swapchain();
    }
    else if (nlines > 0 && ncols > 0){
        // The user wants to resize the terminal programmatically
        PDC_d2d_rows = nlines;
        PDC_d2d_cols = ncols;

        if(PDC_d2d_context == NULL){
            return OK;
        }

        RECT newrect;
        RECT window;
        GetWindowRect(PDC_d2d_hwnd, &window);

        const int client_width = ncols * PDC_D2D_CHAR_WIDTH;
        const int client_height = nlines * PDC_D2D_CHAR_HEIGHT;

        newrect.left = 0;
        newrect.top = 0;
        newrect.right = client_width;
        newrect.bottom = client_height;

        BOOL rc = AdjustWindowRectEx(
            &newrect,
            (DWORD)GetWindowLong(PDC_d2d_hwnd, GWL_STYLE),
            GetMenu(PDC_d2d_hwnd) != NULL,
            (DWORD)GetWindowLong(PDC_d2d_hwnd, GWL_EXSTYLE)
        );
        if (!rc) {
            return ERR;
        }

        rc = SetWindowPos(PDC_d2d_hwnd, HWND_TOP,
            window.left,
            window.top,
            newrect.right - newrect.left,
            newrect.bottom - newrect.top,
            0);

        if(!rc){
            return ERR;
        }

        PDC_d2d_resize_swapchain();
    }

    PDC_LOG((__FUNCTION__ " called\n"));
    return OK;
}

void PDC_restore_screen_mode(int i)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}

void PDC_save_screen_mode(int i)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}


// TODO: Look at this a bit closer
// Visual Studio gets a bit upset if we exit the program
// without cleaning up our com objects.
// Need to figure out how PDCurses calls 
// PDC_scr_close() and PDC_scr_free()
void PDC_scr_close(void)
{
    SafeRelease(&PDC_d2d_font_bitmap);
    SafeRelease(&PDC_d2d_colorEffect);
    SafeRelease(&PDC_d2d_swapChain);
    SafeRelease(&PDC_d2d_context);

    if (PDC_d2d_hwnd != NULL) {
        DestroyWindow(PDC_d2d_hwnd);
        PDC_d2d_hwnd = NULL;
    }

    UnregisterClass(PDC_d2d_classname, HINST_THISCOMPONENT);

    CoUninitialize();

    PDC_LOG(("PDC_scr_close() - called\n"));
}

void PDC_scr_free(void)
{


    PDC_LOG(("PDC_scr_free() - called\n"));
}

int PDC_scr_open(void)
{
    PDC_LOG(("PDC_scr_open() - called\n"));

    if (SP == NULL) {
        PDC_LOG(("SP is NULL ?!?"));
        return ERR;
    }

    if(FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))){
        return ERR;
    }

    // Register the window class.
    WNDCLASSEX window = { sizeof(WNDCLASSEX) };

    memset(&window, 0, sizeof(WNDCLASSEX));
    window.cbSize = sizeof(WNDCLASSEX);
	window.hbrBackground = (HBRUSH) COLOR_WINDOW;
	window.hInstance = HINST_THISCOMPONENT;
	window.lpfnWndProc = PDC_d2d_WndProc;
    window.lpszClassName = PDC_d2d_classname;
	window.style = CS_HREDRAW | CS_VREDRAW;
    window.hCursor = LoadCursor(NULL, IDC_ARROW);
        //LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0,0, LR_DEFAULTSIZE);

    ATOM regd = RegisterClassEx(&window);
    if(regd == 0){
        const DWORD err = GetLastError();
        PDC_LOG(("RegisterClassEx() failed"));
        return ERR;
    }

    RECT rect = {0, 0, 640, 400};

    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);

	PDC_d2d_hwnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
		L"MainWindow",
        L"PDCursesDirect2D",
        WS_OVERLAPPEDWINDOW,
		100, 100,
        rect.right - rect.left,
        rect.bottom - rect.top,
		NULL, NULL,
        HINST_THISCOMPONENT, NULL
	);

    if(PDC_d2d_hwnd == NULL){
        const DWORD err = GetLastError();
        PDC_LOG(("CreateWindowEx() failed with code %d\n", err));
        UnregisterClass(PDC_d2d_classname, HINST_THISCOMPONENT);
        return ERR;
    }

    bool trc = PDC_d2d_create_context();
    if(!trc){
        PDC_scr_free();
        return ERR;
    }

    trc = PDC_d2d_load_font_from_memory();
    if(!trc){
        PDC_scr_free();
        return ERR;
    }

    SP->orig_attr = TRUE;
    SP->orig_fore = COLOR_WHITE;
    SP->orig_back = COLOR_BLACK;

    SP->mono = FALSE;
    SP->audible = FALSE;
    SP->lines = PDC_get_rows();
    SP->cols = PDC_get_columns();

    SP->mouse_wait = PDC_CLICK_PERIOD;

    SP->termattrs = A_COLOR | A_UNDERLINE | A_LEFT | A_RIGHT | A_REVERSE;

    // We need to set this so PDCurses knows how many colors we suppport.
    COLORS = PDC_MAXCOL;

    PDC_d2d_init_events();
    PDC_d2d_init_colours();

    ShowWindow(PDC_d2d_hwnd, SW_SHOWNORMAL);
    UpdateWindow(PDC_d2d_hwnd);

    return OK;
}


// https://docs.microsoft.com/en-us/windows/desktop/direct2d/color-matrix

static bool PDC_d2d_create_context()
{
    // https://msdn.microsoft.com/en-us/magazine/dn198239.aspx
    // https://msdn.microsoft.com/magazine/jj991972
    // https://docs.microsoft.com/en-us/windows/desktop/direct2d/devices-and-device-contexts
    // This is batshit insane.

    D2D1_FACTORY_OPTIONS opts = { D2D1_DEBUG_LEVEL_NONE };

#ifdef _DEBUG
    opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    HRESULT hr;

    ComPtr<ID2D1Factory1> d2dFactory1;
    hr = D2D1CreateFactory<ID2D1Factory1>(
        D2D1_FACTORY_TYPE_SINGLE_THREADED, 
        opts,
        &d2dFactory1
    );
    if (FAILED(hr)) {
        return false;
    }

#if WINVER >= _WIN32_WINNT_WIN10
    PDC_d2d_dpi_y = PDC_d2d_dpi_x = GetDpiForWindow(PDC_d2d_hwnd);
#else
    d2dFactory1->GetDesktopDpi(&PDC_d2d_dpi_x, &PDC_d2d_dpi_y);
#endif
    // This flag adds support for surfaces with a different color channel ordering than the API default.
    // You need it for compatibility with Direct2D.
    const UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT
#ifdef _DEBUG
        | D3D11_CREATE_DEVICE_DEBUG
#endif
        ;

    // This array defines the set of DirectX hardware feature levels this app  supports.
    // The ordering is important and you should  preserve it.
    // Don't forget to declare your app's minimum required feature level in its
    // description.  All apps are assumed to support 9.1 unless otherwise stated.
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    D3D_FEATURE_LEVEL featureLevel;

    // Create the DX11 API device object, and get a corresponding context.

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    hr = D3D11CreateDevice(
            nullptr,                    // specify null to use the default adapter
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            creationFlags,              // optionally set debug and Direct2D compatibility flags
            featureLevels,              // list of feature levels this app can support
            ARRAYSIZE(featureLevels),   // number of possible feature levels
            D3D11_SDK_VERSION,
            &device,                    // returns the Direct3D device created
            &featureLevel,              // returns feature level of device created
            &context                    // returns the device immediate context
        );
    if(FAILED(hr)){
        return false;
    }

    ComPtr<IDXGIDevice1> dxgiDevice;

    hr = device.As(&dxgiDevice);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<ID2D1Device> d2dDevice;
    hr  = d2dFactory1->CreateDevice(dxgiDevice.Get(), &d2dDevice);
    if (FAILED(hr)) {
        return false;
    }
    
    // DXGI_FORMAT_R8G8B8A8_UNORM ?

    // Allocate a descriptor.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
    swapChainDesc.Width = 0;                           // use automatic sizing
    swapChainDesc.Height = 0;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;                // don't use multi-sampling
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;                     // use double buffering to enable flip
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all apps must use this SwapEffect
    swapChainDesc.Flags = 0;

    ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<IDXGIFactory2> dxgiFactory;
    hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        return false;
    }

    hr = dxgiFactory->CreateSwapChainForHwnd(
        device.Get(),
        PDC_d2d_hwnd,
        &swapChainDesc,
        nullptr,    // allow on all displays
        nullptr,
        &PDC_d2d_swapChain
    );
    if (FAILED(hr)) {
        return false;
    }

    hr = dxgiFactory->MakeWindowAssociation(PDC_d2d_hwnd, 0);
    if (FAILED(hr)) {
        return false;
    }

    hr = dxgiDevice->SetMaximumFrameLatency(1);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    hr = PDC_d2d_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        return false;
    }

    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, 
                D2D1_ALPHA_MODE_IGNORE  
                /*D2D1_ALPHA_MODE_PREMULTIPLIED*/),
            PDC_d2d_dpi_x,
            PDC_d2d_dpi_y
        );

    ComPtr<IDXGISurface> dxgiBackBuffer;
    hr = PDC_d2d_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) {
        return false;
    }


    // Need to re-create everything from here onwards in the event of a resize.
    hr = d2dDevice->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        &PDC_d2d_context
    );
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    hr = PDC_d2d_context->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.Get(),
        &bitmapProperties,
        &d2dTargetBitmap
    );
    if (FAILED(hr)) {
        return false;
    }

    PDC_d2d_context->SetTarget(d2dTargetBitmap.Get());

    hr = ColorGlyphEffect::Register(d2dFactory1.Get());
    if (FAILED(hr)) {
        return false;
    }

    hr = PDC_d2d_context->CreateEffect(CLSID_ColorGlyphEffect,
        &PDC_d2d_colorEffect);
    if (FAILED(hr)) {
        return false;
    }

    // We now have a properly configured D2D Device context.
    // before we can use it we need to fill the entire display
    // and present it so that Present1 calls will work when
    // supplied with a rect param that limits the copy area.

    DXGI_PRESENT_PARAMETERS params = {0};
    PDC_d2d_context->BeginDraw();
    PDC_d2d_context->Clear(0x0);
    hr = PDC_d2d_context->EndDraw();

    if (FAILED(hr)) {
        return false;
    }

    hr = PDC_d2d_swapChain->Present1(1,0,&params);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

static D2D1::ColorF make_color(unsigned char r, unsigned char g, unsigned char b)
{
    const UINT32 color = (r << 16) | (g << 8) | b;
    return D2D1::ColorF(color);
}

static void PDC_d2d_init_colours()
{
    // First 8 colours are the standard XTerm colours.
    // Next 8 are the bolded versions.

    const unsigned char normal = 0xC2;
    const unsigned char bold = 0xFF;

    for(int i = 0; i < 8; i++){
        const unsigned char r = (i & COLOR_RED)   ? normal : 0;
        const unsigned char g = (i & COLOR_GREEN) ? normal : 0;
        const unsigned char b = (i & COLOR_BLUE)  ? normal : 0;

        const unsigned char rb = (i & COLOR_RED)   ? bold : 0;
        const unsigned char gb = (i & COLOR_GREEN) ? bold : 0;
        const unsigned char bb = (i & COLOR_BLUE)  ? bold : 0;

        PDC_d2d_colors[i] = make_color(r, g, b);
        PDC_d2d_colors[i + 8] = make_color(rb, gb, bb);
    }

    // The XTerm extended colour set.
    int color = 16;
    for(int r = 0; r < 6; r++){
        for(int g = 0; g < 6; g++){
            for(int b = 0; b < 6; b++){
                const unsigned char red = (r ? r * 40 + 55 : 0);
                const unsigned char grn = (g ? g * 40 + 55 : 0);
                const unsigned char blu = (b ? b * 40 + 55 : 0);

                PDC_d2d_colors[color++] = make_color(red, grn, blu);
            }
        }
    }

    // grey colours
    for(int i = 0; i < 24; i++){
        const unsigned char value = i * 10 + 8;
        PDC_d2d_colors[i + 232] = make_color(value, value, value);
    }
}

// https://i.imgur.com/U4GnISB.jpg 

static bool PDC_d2d_load_font_from_file()
{
    ComPtr<IWICBitmapDecoder> pDecoder;
    ComPtr<IWICBitmapFrameDecode> pSource;
    ComPtr<IWICFormatConverter> pConverter;
    ComPtr<IWICImagingFactory> pIWICFactory;

    HRESULT hr;
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        &pIWICFactory
    );

    hr = pIWICFactory->CreateDecoderFromFilename(
        L"D:\\Programming\\projects\\font.bmp",
        NULL,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &pDecoder
    );

    if (SUCCEEDED(hr))
    {
        // Create the initial frame.
        hr = pDecoder->GetFrame(0, &pSource);
    }

    if (SUCCEEDED(hr))
    {
        // Convert the image format to 32bppPBGRA
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }

    if (SUCCEEDED(hr))
    {
        hr = pConverter->Initialize(
            pSource.Get(),
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeMedianCut
        );
    }

    if (SUCCEEDED(hr))
    {

        // Create a Direct2D bitmap from the WIC bitmap.
        hr = PDC_d2d_context->CreateBitmapFromWicBitmap(
            pConverter.Get(),
            NULL,
            &PDC_d2d_font_bitmap
        );
    }

    // todo fix this?
    PDC_d2d_colorEffect->SetInput(0, PDC_d2d_font_bitmap);

    return SUCCEEDED(hr);
}

static bool PDC_d2d_load_font_from_memory()
{
    ComPtr<IWICFormatConverter> pConverter;
    ComPtr<IWICImagingFactory> pIWICFactory;
    ComPtr<IWICPalette> pPalette;
    ComPtr<IWICBitmap> pBitmap;
    ComPtr<IWICBitmapFlipRotator> pIFlipRotator;
    ComPtr<IWICBitmapDecoder> pIDecoder;

    HRESULT hr;

    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        &pIWICFactory
    );

    // We need to skip the bitmap header and provide the raw pixel data
    // To do this we grab the offset where the data begins from the bitmap header
    BITMAPFILEHEADER *fh = (BITMAPFILEHEADER*) deffont;

    if(SUCCEEDED(hr)){
        hr = pIWICFactory->CreateBitmapFromMemory(
            256, 128,
            GUID_WICPixelFormat1bppIndexed,
            32, // 256 pixels wide, one bit per pixel. a scan line is 32 bytes long.
            sizeof(deffont) - fh->bfOffBits,
            deffont + fh->bfOffBits,  // skip bitmap header
            &pBitmap
        );
    }

    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreatePalette(&pPalette);
    }

    if (SUCCEEDED(hr)) {
        hr = pPalette->InitializePredefined(WICBitmapPaletteTypeFixedBW, FALSE);
    }
    
    if(SUCCEEDED(hr)){
        hr = pBitmap->SetPalette(pPalette.Get());
    }

    // For some reason when loading from memory the image is upside down.
    // I have no idea why that is the case, but we flip it 180 to fix that.

    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateDecoder(GUID_ContainerFormatBmp, NULL, &pIDecoder);
    }

    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateBitmapFlipRotator(&pIFlipRotator);
    }

    if(SUCCEEDED(hr)){
        hr = pIFlipRotator->Initialize(pBitmap.Get(), WICBitmapTransformFlipVertical);
    }

    if (SUCCEEDED(hr)) {
        // Convert bitmap to 32bppPBGRA so Direct2D can use it.
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }

    if(SUCCEEDED(hr)){
        hr = pConverter->Initialize(
            pIFlipRotator.Get(),
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeFixedBW
        );
    }

    if(SUCCEEDED(hr)){
        // Create a Direct2D bitmap from the WIC bitmap.
        hr = PDC_d2d_context->CreateBitmapFromWicBitmap(
            pConverter.Get(),
            NULL,
            &PDC_d2d_font_bitmap
        );
    }

    PDC_d2d_colorEffect->SetInput(0, PDC_d2d_font_bitmap);

    return SUCCEEDED(hr);
}


// https://msdn.microsoft.com/en-us/magazine/dn198239.aspx
// I need to release and re-create the Direct2D render target so that it
// matches the size of the new window area.

// Must be called whenever the display is resized
static bool PDC_d2d_resize_swapchain(void)
{
    ULONG ul;
    HRESULT hr;

    // Release the old render target
    PDC_d2d_context->SetTarget(nullptr);

    // Resize swapchain.
    hr = PDC_d2d_swapChain->ResizeBuffers(0,
        PDC_d2d_cols * PDC_D2D_CHAR_WIDTH,
        PDC_D2D_CHAR_HEIGHT * PDC_d2d_rows,
        DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        return false;
    }

    // Create a new render target.
    ComPtr<IDXGISurface> dxgiBackBuffer;
    hr = PDC_d2d_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) {
        return false;
    }

    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE
            /*D2D1_ALPHA_MODE_PREMULTIPLIED*/),
            PDC_d2d_dpi_x,
            PDC_d2d_dpi_y
        );

    ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    hr = PDC_d2d_context->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.Get(),
        &bitmapProperties,
        &d2dTargetBitmap
    );
    if (FAILED(hr)) {
        return false;
    }

    PDC_d2d_context->SetTarget(d2dTargetBitmap.Get());
    PDC_d2d_context->BeginDraw();
    PDC_d2d_context->Clear(0);
    hr = PDC_d2d_context->EndDraw();
    if (FAILED(hr)) {
        return false;
    }

    hr = PDC_d2d_swapChain->Present(1, 0);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}
