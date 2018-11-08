#include "pdcd2d.h"

#include "deffont.h"

#include "ColorGlyphEffect.hpp"

#include <memory>

#include <d3d11.h>
#include <wrl/client.h>
using namespace Microsoft::WRL; // for ComPtr

HWND hwnd = NULL;
ID2D1DeviceContext *m_d2dContext = NULL;
ID2D1Effect *pdc_colorEffect = NULL;
D2D1::ColorF pdc_d2d_colors[];
ID2D1Bitmap *pdc_font_bitmap = NULL;
IDXGISwapChain1 *m_swapChain = NULL;


static struct {
    short fg;
    short bg;
}
atrtab [PDC_COLOR_PAIRS];

static void d2d_init_colours();
static bool d2d_load_font_from_memory();
static bool d2d_load_font_from_file();
static bool d2d_create_context();

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

void PDC_init_pair(short pair, short fg, short bg)
{
    atrtab[pair].fg = fg;
    atrtab[pair].bg = bg;
    PDC_LOG((__FUNCTION__ " called\n"));
}

int PDC_pair_content(short pair, short *fg, short *bg)
{
    *fg = atrtab[pair].fg;
    *bg = atrtab[pair].bg;
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
    RECT rect = {0, 0, 640, 400};

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
    SafeRelease(&pdc_font_bitmap);
    SafeRelease(&m_d2dContext);
    SafeRelease(&pdc_colorEffect);
    SafeRelease(&m_swapChain);

    if (hwnd != NULL) {
        DestroyWindow(hwnd);
        hwnd = NULL;
    }

    CoUninitialize();

    PDC_LOG(("PDC_scr_close() - called\n"));
}

void PDC_scr_free(void)
{
    if(SP){
        free(SP);
        SP = NULL;
    }
}
#include <WinUser.h>
int PDC_scr_open(int argc, char **argv)
{
    PDC_LOG(("PDC_scr_open() - called\n"));

    if(SP != NULL){
        free(SP);
    }
    SP = (SCREEN*) calloc(1, sizeof(SCREEN));

    if(FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))){
        return -1;
    }

    // Register the window class.
    WNDCLASSEX window = { sizeof(WNDCLASSEX) };

    memset(&window, 0, sizeof(WNDCLASSEX));
    window.cbSize = sizeof(WNDCLASSEX);
	window.hbrBackground = (HBRUSH) COLOR_WINDOW;
	window.hInstance = HINST_THISCOMPONENT;
	window.lpfnWndProc = PDC_d2d_WndProc;
	window.lpszClassName = L"MainWindow";
	window.style = CS_HREDRAW | CS_VREDRAW;
    window.hCursor = LoadCursor(NULL, IDC_ARROW);
        //LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0,0, LR_DEFAULTSIZE);

    ATOM regd = RegisterClassEx(&window);
    if(regd == 0){
        const DWORD err = GetLastError();
        PDC_LOG(("RegisterClassEx() failed"));
        return -1;
    }

    RECT rect = {0, 0, 640, 400};

    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);

	hwnd = CreateWindowEx(
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

    if(hwnd == NULL){
        const DWORD err = GetLastError();
        PDC_LOG(("CreateWindowEx() failed with code %d\n", err));
        UnregisterClass(L"MainWindow", HINST_THISCOMPONENT);
        return -1;
    }

    bool trc = d2d_create_context();
    if(!trc){
        PDC_scr_free();
        return -1;
    }

    trc = d2d_load_font_from_memory();
    if(!trc){
        PDC_scr_free();
        return -1;
    }

    d2d_init_colours();

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    SP->orig_attr = TRUE;
    SP->orig_fore = COLOR_WHITE;
    SP->orig_back = -1;

    SP->mono = FALSE;
    SP->audible = FALSE;
    SP->lines = PDC_get_rows();
    SP->cols = PDC_get_columns();

    SP->audible = FALSE;
    SP->mouse_wait = PDC_CLICK_PERIOD;

    SP->termattrs = A_COLOR | A_UNDERLINE | A_LEFT | A_RIGHT | A_REVERSE;

    // We need to set this so PDCurses knows how many colors we suppport.
    COLORS = 256;

    PDC_d2d_init_events();

    return OK;
}


// https://docs.microsoft.com/en-us/windows/desktop/direct2d/color-matrix



static bool d2d_create_context()
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

    hr = d2dDevice->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        &m_d2dContext
    );
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
        hwnd,
        &swapChainDesc,
        nullptr,    // allow on all displays
        nullptr,
        &m_swapChain
    );
    if (FAILED(hr)) {
        return false;
    }

    hr = dxgiDevice->SetMaximumFrameLatency(1);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        return false;
    }

    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, 
                D2D1_ALPHA_MODE_IGNORE  
                /*D2D1_ALPHA_MODE_PREMULTIPLIED*/),
            96.0f,
            96.0f
        );

    ComPtr<IDXGISurface> dxgiBackBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    hr = m_d2dContext->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.Get(),
        &bitmapProperties,
        &d2dTargetBitmap
    );
    if (FAILED(hr)) {
        return false;
    }

    m_d2dContext->SetTarget(d2dTargetBitmap.Get());

    hr = ColorGlyphEffect::Register(d2dFactory1.Get());
    if (FAILED(hr)) {
        return false;
    }

    hr = m_d2dContext->CreateEffect(CLSID_ColorGlyphEffect,
        &pdc_colorEffect);
    if (FAILED(hr)) {
        return false;
    }

    // We now have a properly configured D2D Device context.
    // before we can use it we need to fill the entire display
    // and present it so that Present1 calls will work when
    // supplied with a rect param that limits the copy area.

    DXGI_PRESENT_PARAMETERS params = {0};
    m_d2dContext->BeginDraw();
    m_d2dContext->Clear(0x0);
    hr = m_d2dContext->EndDraw();

    if (FAILED(hr)) {
        return false;
    }

    hr = m_swapChain->Present1(1,0,&params);
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

static void d2d_init_colours()
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

        pdc_d2d_colors[i] = make_color(r, g, b);
        pdc_d2d_colors[i + 8] = make_color(rb, gb, bb);
    }

    // The XTerm extended colour set.
    int color = 16;
    for(int r = 0; r < 6; r++){
        for(int g = 0; g < 6; g++){
            for(int b = 0; b < 6; b++){
                const unsigned char red = (r ? r * 40 + 55 : 0);
                const unsigned char grn = (g ? g * 40 + 55 : 0);
                const unsigned char blu = (b ? b * 40 + 55 : 0);

                pdc_d2d_colors[color++] = make_color(red, grn, blu);
            }
        }
    }

    // grey colours
    for(int i = 0; i < 24; i++){
        const unsigned char value = i * 10 + 8;
        pdc_d2d_colors[i + 232] = make_color(value, value, value);
    }
}

// https://i.imgur.com/U4GnISB.jpg 

static bool d2d_load_font_from_file()
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
        hr = m_d2dContext->CreateBitmapFromWicBitmap(
            pConverter.Get(),
            NULL,
            &pdc_font_bitmap
        );
    }

    // todo fix this?
    pdc_colorEffect->SetInput(0, pdc_font_bitmap);

    return SUCCEEDED(hr);
}

static bool d2d_load_font_from_memory()
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
        hr = m_d2dContext->CreateBitmapFromWicBitmap(
            pConverter.Get(),
            NULL,
            &pdc_font_bitmap
        );
    }

    pdc_colorEffect->SetInput(0, pdc_font_bitmap);

    return SUCCEEDED(hr);
}
