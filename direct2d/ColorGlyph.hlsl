
#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_SIMPLE

// Note that the custom build step must provide the correct path to find d2d1effecthelpers.hlsli when calling fxc.exe.
#include <d2d1effecthelpers.hlsli>


// fxc /Tlib_5_0 /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17134.0\um" /Flshader.fxlib /DD2D_FUNCTION /DD2D_ENTRY=ColorChangeShader  ColorGlyph.hlsl
// fxc /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17134.0\um" /T ps_5_0 ColorGlyph.hlsl /DD2D_FULL_SHADER /DD2D_ENTRY=ColorChangeShader /EColorChangeShader /setprivate shader.fxlib /Fh myshader.h


// http://divilopment.blogspot.com/2015/06/hlsl-and-cmake.html
// https://docs.microsoft.com/en-us/windows/desktop/direct2d/effect-shader-linking

//  dxc -T ps_5_0 ColorGlyph.hlsl


cbuffer constants : register(b0)
{
    float fg_r : packoffset(c0.x);
    float fg_g : packoffset(c0.y);
    float fg_b : packoffset(c0.z);

    float bg_r : packoffset(c0.w);
    float bg_g : packoffset(c1.x);
    float bg_b : packoffset(c1.y);
};

D2D_PS_ENTRY(ColorChangeShader)
{
    float4 output = D2DGetInput(0);

    if(
        output.r == 1.0f &&
        output.g == 1.0f &&
        output.b == 1.0f
    ){
        output.r = fg_r;
        output.g = fg_g;
        output.b = fg_b;
    }
    else if (!all(output)) {
        output.r = bg_r;
        output.g = bg_g;
        output.b = bg_b;
    }
    
    return output;
}
